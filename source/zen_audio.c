/* zen_audio.c -- ZENONIA 3's sound: what NexusSound did (a SoundPool, and two
 * MediaPlayers), on the runtime's audout pump.
 *
 * The engine asks Natives.OnSoundPlay(id, volume, loop). In the Java:
 *   volume 0      the sound is off: with loop it also stops the music; no sound
 *   id 1..15      NexusSound.playSFXSound: a SoundPool of 3 streams over the 15
 *                 clips the game loaded at start-up
 *   any other id  the raw resource 0x7f050000 + id, then playSound(res, loop):
 *                 a looping one is the music (one at a time, a new one
 *                 replaces it), a non-looping one the "one clip at a time"
 *                 player (a new one replaces it)
 * Natives.OnStopSound stops everything. zen_sounds.h names the 76 files.
 *
 * The files are MP3 (22.05 kHz mono, 44.1 kHz mono and stereo): each is
 * decoded with minimp3 (public domain, minimp3.h / minimp3_ex.h) the first
 * time the game asks for it -- on the game's own thread, which is the only one
 * that reads the APK (zen_apk.c) -- and kept as 16-bit PCM; the mixer thread
 * only reads finished clips. It mixes 5 voices (music, one-clip, 3 effects) to
 * 48 kHz stereo with linear interpolation. The volume the engine passes went
 * through NexusSound.setVolume: v / 10, clamped at 1 by MediaPlayer, so 10 or
 * more is full; here it is v / 10 as well, times config.ini's [audio]
 * volume. MIT.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#define MINIMP3_IMPLEMENTATION
#define MINIMP3_NO_STDIO
#define MINIMP3_NO_SIMD /* a plain C decoder: the clips are small and decoded once */
#include "minimp3_ex.h"

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
  int16_t *pcm; /* interleaved when stereo; malloc'd; NULL: not decoded (yet) */
  uint32_t frames, rate;
  int channels;
  int tried; /* decoding was attempted (it failed if pcm is NULL) */
} Clip;

typedef struct {
  const Clip *clip; /* NULL: free */
  uint64_t pos;     /* 32.32 fixed point, in the clip's frames */
  int loop;
  int gain; /* 0..256 */
} Voice;

static Clip g_clip[ZEN_NFILES];
static Voice g_voice[NVOICES];
static Mutex g_lock;
static volatile unsigned long g_mixes;
static int g_volume = 256; /* config.ini: 0..256 */
static int g_up;
static int g_next_sfx;
static mp3dec_t g_dec; /* game thread only */

unsigned long zen_audio_mixes(void) { return g_mixes; }

/* ------------------------------------------------------------------ decode */
/* the clip behind sound number idx: decoded now if it is the first time */
static const Clip *clip_of(int idx) {
  if (idx < 0 || idx >= ZEN_NFILES)
    return NULL;
  Clip *c = &g_clip[idx];
  if (c->tried)
    return c->pcm ? c : NULL;
  c->tried = 1;
  size_t n = 0;
  uint8_t *d = zen_apk_read(k_zen_files[idx], &n);
  if (!d) {
    debugPrintf("[audio] %s: not in the APK\n", k_zen_files[idx]);
    return NULL;
  }
  mp3dec_file_info_t info;
  memset(&info, 0, sizeof info);
  mp3dec_init(&g_dec);
  int rc = mp3dec_load_buf(&g_dec, d, n, &info, NULL, NULL);
  free(d);
  if (rc || !info.buffer || info.samples == 0 || (info.channels != 1 && info.channels != 2) || info.hz < 4000) {
    debugPrintf("[audio] %s: not an MP3 I can decode (rc %d)\n", k_zen_files[idx], rc);
    free(info.buffer);
    return NULL;
  }
  c->pcm = info.buffer;
  c->channels = info.channels;
  c->rate = (uint32_t)info.hz;
  c->frames = (uint32_t)(info.samples / (size_t)info.channels);
  return c;
}

/* ------------------------------------------------------------------ mixer */
static void fill(int16_t *out, int frames, void *ud) {
  (void)ud;
  const uint32_t out_rate = rt_audout_rate();
  int32_t accl[RT_AUDOUT_FRAMES], accr[RT_AUDOUT_FRAMES];
  if (frames > RT_AUDOUT_FRAMES)
    frames = RT_AUDOUT_FRAMES;
  memset(accl, 0, sizeof(int32_t) * (size_t)frames);
  memset(accr, 0, sizeof(int32_t) * (size_t)frames);
  mutexLock(&g_lock);
  for (int v = 0; v < NVOICES; v++) {
    Voice *vo = &g_voice[v];
    if (!vo->clip)
      continue;
    const Clip *c = vo->clip;
    const int st = c->channels == 2;
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
      int32_t l0 = c->pcm[i0 << st], l1 = c->pcm[i1 << st];
      int32_t l = l0 + (((l1 - l0) * f) >> 16);
      int32_t r = l;
      if (st) {
        int32_t r0 = c->pcm[(i0 << 1) + 1], r1 = c->pcm[(i1 << 1) + 1];
        r = r0 + (((r1 - r0) * f) >> 16);
      }
      accl[i] += (l * vo->gain) >> 8;
      accr[i] += (r * vo->gain) >> 8;
      vo->pos += step;
    }
  }
  int vol = g_volume;
  mutexUnlock(&g_lock);
  for (int i = 0; i < frames; i++) {
    int32_t l = (accl[i] * vol) >> 8, r = (accr[i] * vol) >> 8;
    if (l > 32767)
      l = 32767;
    else if (l < -32768)
      l = -32768;
    if (r > 32767)
      r = 32767;
    else if (r < -32768)
      r = -32768;
    out[i * 2] = (int16_t)l;
    out[i * 2 + 1] = (int16_t)r;
  }
  g_mixes++;
}

/* ------------------------------------------------------------------ the API */
int zen_audio_init(void) {
  mutexInit(&g_lock);
  g_volume = zen_config()->volume * 256 / 100;
  if (rt_audout_open() != 0)
    return -1;
  if (rt_audout_pump_start(fill, NULL, 0x2A, -2) != 0)
    return -1;
  g_up = 1;
  debugPrintf("[audio] %d MP3 sounds, decoded when first played\n", ZEN_NFILES);
  return 0;
}

static void start(int voice, const Clip *c, int loop, int gain) {
  Voice *vo = &g_voice[voice];
  vo->clip = c;
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
  const Clip *c = clip_of(id); /* the sound number is the raw resource index */
  if (!c) {
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
    start(v, c, 0, gain);
  } else if (loop) {
    start(V_BGM, c, 1, gain);
  } else {
    start(V_ONE, c, 0, gain);
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
