/* zen_audio.c -- ZENONIA 2's sound: what NexusSound did (a SoundPool, and two
 * MediaPlayers), on the runtime's audout pump.
 *
 * The engine asks Natives.OnSoundPlay(id, volume, loop). In the Java:
 *   volume 0      the sound is off: with loop it also stops the music; no sound
 *   id 1..15      NexusSound.playSFXSound: a SoundPool of 3 streams over the 15
 *                 clips the game loaded at start-up (13 is played as 12)
 *   any other id  getAudioResource(id) -> a raw resource, then playSound(res,
 *                 loop): a looping one is the music (one at a time, a new one
 *                 replaces it), a non-looping one the "one clip at a time"
 *                 player (a new one replaces it)
 * Natives.OnStopSound / stopAndroidSound stop everything.
 * zen_sounds.h has the tables, read from the game's own code and resources.
 *
 * All 74 files are 16-bit mono PCM at 22050 Hz: they are decoded once at
 * start-up from the APK (the game's thread, zen_apk.c), and the mixer thread
 * only reads them. It mixes 5 voices (music, one-clip, 3 effects) to 48 kHz
 * stereo with linear interpolation. The volume the engine passes went through
 * NexusSound.setVolume: v / 10, clamped at 1 by MediaPlayer, so 10 or more is
 * full; here it is v / 10 as well, times config.ini's [audio] volume. MIT.
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

#define V_BGM 0
#define V_ONE 1
#define V_SFX0 2
#define NSFX 3
#define NVOICES (V_SFX0 + NSFX)

typedef struct {
  int16_t *pcm; /* mono, malloc'd; NULL: not decoded */
  uint32_t frames, rate;
} Clip;

typedef struct {
  const Clip *clip; /* NULL: free */
  uint64_t pos;     /* 32.32 fixed point, in the clip's frames */
  int loop;
  int gain; /* 0..256 */
} Voice;

static Clip g_clip[ZEN_NWAVS];
static Voice g_voice[NVOICES];
static Mutex g_lock;
static volatile unsigned long g_mixes;
static int g_volume = 256; /* config.ini: 0..256 */
static int g_up;
static int g_next_sfx;

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
      acc[i] += (s * vo->gain) >> 8;
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

static int wav_of_audio_id(int id) {
  for (int i = 0; i < ZEN_NAUDIO; i++)
    if (k_zen_audio[i].id == id)
      return k_zen_audio[i].wav;
  return -1;
}

static void start(int voice, int wav, int loop, int gain) {
  Voice *vo = &g_voice[voice];
  vo->clip = &g_clip[wav];
  vo->pos = 0;
  vo->loop = loop;
  vo->gain = gain;
}

void zen_audio_play(int id, int volume, int loop) {
  if (!g_up)
    return;
  if (volume <= 0) { /* NexusSound.setVolume(0): sound off; a looping request also stops the music */
    if (loop) {
      mutexLock(&g_lock);
      g_voice[V_BGM].clip = NULL;
      mutexUnlock(&g_lock);
    }
    return;
  }
  int gain = volume >= 10 ? 256 : volume * 256 / 10;
  int wav;
  if (id >= 1 && id <= 15) {
    wav = k_zen_sfx[id == 13 ? 12 : id];
  } else {
    wav = wav_of_audio_id(id);
  }
  if (wav < 0 || wav >= ZEN_NWAVS || !g_clip[wav].pcm) {
    debugPrintf("[audio] play(%d): no such sound\n", id);
    return;
  }
  mutexLock(&g_lock);
  if (id >= 1 && id <= 15) {
    /* the SoundPool: 3 streams, the oldest gives way */
    int v = V_SFX0 + g_next_sfx;
    for (int k = 0; k < NSFX; k++) {
      if (!g_voice[V_SFX0 + k].clip) {
        v = V_SFX0 + k;
        break;
      }
    }
    g_next_sfx = (g_next_sfx + 1) % NSFX;
    start(v, wav, 0, gain);
  } else if (loop) {
    start(V_BGM, wav, 1, gain);
  } else {
    start(V_ONE, wav, 0, gain);
  }
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
