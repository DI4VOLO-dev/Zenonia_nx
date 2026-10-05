/* zen_audio.c -- ZENONIA's sound: what NexusSound / AudioClip / MediaPlayer did,
 * on the runtime's audout pump.
 *
 * The engine asks Natives.OnSoundPlay(id, volume, loop). The Java picked a
 * raw resource by the id (ids 0-41 as they are, 100 and up minus 58: the 51
 * WAV files of res/raw, in zen_sounds.h), and played it: a looping one was the
 * background music (one at a time, a new one replaced it), the rest were
 * effects on a few channels (a repeated effect restarted its own clip).
 * Natives.OnStopSound stopped everything.
 *
 * All 51 files are 16-bit mono PCM at 22050 Hz, 7.9 MB in all: they are
 * decoded once at start-up from the APK (the game's thread, zen_apk.c), and
 * the mixer thread only reads them. It mixes up to 12 voices to 48 kHz stereo
 * with linear interpolation. The Java's volume argument went through an
 * integer divide by 10 before MediaPlayer.setVolume (which clamps at 1), so
 * any volume of 10 or more was full and the smaller ones silent: here zero is
 * silent and the rest full, times config.ini's [audio] volume. MIT.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#include "rt_audout.h"
#include "util.h"
#include "zen.h"
#include "zen_config.h"
#include "zen_sounds.h"

#define NVOICES 12
#define BGM_VOICE 0

typedef struct {
  int16_t *pcm; /* mono, malloc'd; NULL: not decoded */
  uint32_t frames, rate;
} Clip;

typedef struct {
  const Clip *clip; /* NULL: free */
  uint64_t pos;     /* 32.32 fixed point, in the clip's frames */
  int loop;
  int id;
} Voice;

static Clip g_clip[ZEN_NWAVS];
static Voice g_voice[NVOICES];
static Mutex g_lock;
static volatile unsigned long g_mixes;
static int g_volume = 256; /* 0..256 */
static int g_up;

unsigned long zen_audio_mixes(void) { return g_mixes; }

/* ------------------------------------------------------------------ decode */
static uint32_t rd32(const uint8_t *p) { return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24; }
static uint16_t rd16(const uint8_t *p) { return (uint16_t)(p[0] | p[1] << 8); }

/* RIFF/WAVE PCM, 16-bit (8-bit too), mono or stereo -> mono s16 */
static int decode_wav(const uint8_t *d, size_t n, Clip *c) {
  if (n < 12 || memcmp(d, "RIFF", 4) || memcmp(d + 8, "WAVE", 4))
    return -1;
  unsigned fmt_tag = 0, ch = 0, bits = 0;
  uint32_t rate = 0;
  const uint8_t *data = NULL;
  size_t dlen = 0;
  for (size_t p = 12; p + 8 <= n;) {
    uint32_t len = rd32(d + p + 4);
    const uint8_t *body = d + p + 8;
    if (!memcmp(d + p, "fmt ", 4) && p + 8 + 16 <= n) {
      fmt_tag = rd16(body);
      ch = rd16(body + 2);
      rate = rd32(body + 4);
      bits = rd16(body + 14);
    } else if (!memcmp(d + p, "data", 4)) {
      data = body;
      dlen = len;
      if (p + 8 + dlen > n)
        dlen = n - p - 8;
      break;
    }
    p += 8 + (size_t)len + (len & 1);
  }
  if (!data || fmt_tag != 1 || (ch != 1 && ch != 2) || (bits != 16 && bits != 8) || rate < 4000)
    return -1;
  size_t bytes_per = (bits / 8) * ch;
  uint32_t frames = (uint32_t)(dlen / bytes_per);
  int16_t *pcm = malloc((size_t)frames * 2 + 2);
  if (!pcm)
    return -1;
  for (uint32_t i = 0; i < frames; i++) {
    int acc = 0;
    for (unsigned k = 0; k < ch; k++) {
      const uint8_t *s = data + (size_t)i * bytes_per + k * (bits / 8);
      acc += bits == 16 ? (int16_t)rd16(s) : ((int)s[0] - 128) * 256;
    }
    pcm[i] = (int16_t)(acc / (int)ch);
  }
  c->pcm = pcm;
  c->frames = frames;
  c->rate = rate;
  return 0;
}

/* ------------------------------------------------------------------ mixer */
static void fill(int16_t *out, int frames, void *ud) {
  (void)ud;
  const uint32_t out_rate = rt_audout_rate();
  int32_t acc[RT_AUDOUT_FRAMES];
  if (frames > RT_AUDOUT_FRAMES)
    frames = RT_AUDOUT_FRAMES;
  memset(acc, 0, sizeof(int32_t) * (size_t)frames);
  mutexLock(&g_lock);
  for (int v = 0; v < NVOICES; v++) {
    Voice *vo = &g_voice[v];
    if (!vo->clip)
      continue;
    const Clip *c = vo->clip;
    const uint64_t step = ((uint64_t)c->rate << 32) / out_rate;
    const uint64_t end = (uint64_t)c->frames << 32;
    for (int i = 0; i < frames; i++) {
      if (vo->pos >= end) {
        if (!vo->loop) {
          vo->clip = NULL;
          break;
        }
        vo->pos -= end;
      }
      uint32_t i0 = (uint32_t)(vo->pos >> 32);
      uint32_t i1 = i0 + 1 < c->frames ? i0 + 1 : (vo->loop ? 0 : i0);
      int32_t f = (int32_t)((vo->pos >> 16) & 0xffff);
      int32_t s = c->pcm[i0] + (((c->pcm[i1] - c->pcm[i0]) * f) >> 16);
      acc[i] += s;
      vo->pos += step;
    }
  }
  int vol = g_volume;
  mutexUnlock(&g_lock);
  for (int i = 0; i < frames; i++) {
    int32_t s = (acc[i] * vol) >> 8;
    if (s > 32767)
      s = 32767;
    else if (s < -32768)
      s = -32768;
    out[i * 2] = out[i * 2 + 1] = (int16_t)s;
  }
  g_mixes++;
}

/* ------------------------------------------------------------------ the API */
int zen_audio_init(void) {
  mutexInit(&g_lock);
  g_volume = zen_config()->volume * 256 / 100;
  int ok = 0;
  for (int i = 0; i < ZEN_NWAVS; i++) {
    size_t n = 0;
    uint8_t *d = zen_apk_read(k_zen_wavs[i], &n);
    if (!d) {
      debugPrintf("[audio] %s: not in the APK\n", k_zen_wavs[i]);
      continue;
    }
    if (decode_wav(d, n, &g_clip[i]) == 0)
      ok++;
    else
      debugPrintf("[audio] %s: not a PCM WAV I can play\n", k_zen_wavs[i]);
    free(d);
  }
  debugPrintf("[audio] %d of %d sounds decoded\n", ok, ZEN_NWAVS);
  if (rt_audout_open() != 0)
    return -1;
  if (rt_audout_pump_start(fill, NULL, 0x2A, -2) != 0)
    return -1;
  g_up = 1;
  return 0;
}

void zen_audio_play(int id, int volume, int loop) {
  if (!g_up)
    return;
  int idx = id >= 100 ? id - 58 : id;
  if (idx < 0 || idx >= ZEN_NWAVS || !g_clip[idx].pcm) {
    debugPrintf("[audio] play(%d): no such sound\n", id);
    return;
  }
  if (volume <= 0)
    return;
  mutexLock(&g_lock);
  Voice *vo = NULL;
  if (loop) {
    vo = &g_voice[BGM_VOICE]; /* the music: a new one replaces it */
  } else {
    for (int v = 1; v < NVOICES; v++) {
      if (g_voice[v].clip && g_voice[v].id == id) { /* the same effect again: restarts its clip */
        vo = &g_voice[v];
        break;
      }
    }
    for (int v = 1; !vo && v < NVOICES; v++)
      if (!g_voice[v].clip)
        vo = &g_voice[v];
    if (!vo) /* all busy: the oldest-started effect gives way (voice 1 is as good as any) */
      vo = &g_voice[1];
  }
  vo->clip = &g_clip[idx];
  vo->pos = 0;
  vo->loop = loop;
  vo->id = id;
  mutexUnlock(&g_lock);
}

void zen_audio_stop_all(void) {
  if (!g_up)
    return;
  mutexLock(&g_lock);
  for (int v = 0; v < NVOICES; v++)
    g_voice[v].clip = NULL;
  mutexUnlock(&g_lock);
}

void zen_audio_pause(int paused) {
  if (g_up)
    rt_audout_pause(paused);
}
