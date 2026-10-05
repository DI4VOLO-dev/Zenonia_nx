/* zen.h -- what the ZENONIA wrapper's files share. MIT. */
#ifndef ZEN_H
#define ZEN_H
#include <stddef.h>
#include <stdint.h>

#include "jni.h"
#include "so_util.h"

#define ZEN_NATIVES "com/gamevil/nexus2/Natives"

/* zen_loader.c: the game library */
extern so_module g_mod_game;
int zen_load_module(void);
void zen_run_constructors(void);
/* Java_com_gamevil_nexus2_Natives_<name>, or NULL */
void *zen_native(const char *name);

/* zen_apk.c: the APK's assets/ and res/ */
int zen_apk_init(const char *apk);
/* The entry's bytes in a malloc'd block (free() it); NULL if there is none. */
uint8_t *zen_apk_read(const char *entry, size_t *len);
/* The entry's size, or -1. */
long zen_apk_size(const char *entry);

/* zen_gfx.c: the screen */
int zen_gfx_init(void);
void zen_gfx_size(int *w, int *h);
void zen_gfx_present(void);

/* zen_input.c: the controller and the touch screen, as the Java views' events */
void zen_input_init(void);
void zen_input_poll(void);   /* the pad and the touch screen -> the event queue */
void zen_input_frame(void);  /* before each frame: one queued event -> handleCletEvent */
void zen_clet_event(int type, int p1, int p2, int pid); /* an event at once (the pause events) */

/* zen_patch.c: the touch controls the game draws itself */
int zen_hide_touch_ui(void);

/* zen_audio.c: MediaPlayer's job, on audout */
int zen_audio_init(void);
void zen_audio_play(int id, int volume, int loop);
void zen_audio_stop_all(void);
void zen_audio_pause(int paused);
unsigned long zen_audio_mixes(void);

/* zen_java.c: the Java side the engine calls */
void zen_java_init(void);
int zen_fps(void);        /* the frame rate the engine asked for (SetSpeed(n): n) */
int zen_smooth(void);     /* config.ini [display] smooth, as getGLOptionLinear */

#endif /* ZEN_H */
