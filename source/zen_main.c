/* zen_main.c -- ZENONIA's part of the boot sequence, and the frame loop.
 *
 * The runtime's main.c (runtime/source/main.c) does what every port does: the
 * log, config.ini, the clocks, HOME / sleep, an update from the launcher NRO,
 * the APK (any *.apk that is ZENONIA's, game.apk first, else the newest), the
 * self-tests. Here is what is the game's:
 *   port_load  the library and classes.txt from the APK (the setup plan
 *              below), the APK's assets and sounds, the library loaded;
 *   port_run   what NexusGLActivity / NexusGLRenderer / NexusGLThread did.
 *
 * Android's order, and what happens here:
 *   Activity.onResume   NativeResumeClet, InitializeJNIGlobalRef
 *   surfaceCreated      NativeInit        (the EGL context is current by then)
 *   surfaceChanged      NativeResize(w, h) -- the surface's own size: the
 *                       engine lays its picture out for any size
 *   every frame         setTouchEvent's pending event -> handleCletEvent,
 *                       NativeRender, then the swap; the thread slept to the
 *                       period Natives.SetSpeed set (1000 / (n + 2) ms)
 *   onPause / onResume  NativePauseClet / NativeResumeClet, sounds stopped
 *   onDestroy           NativeDestroyClet
 * One thread does it all (the GL thread's job); the sound mixer has its own
 * (zen_audio.c). MIT.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#include "config.h"
#include "dcr_apkcache.h"
#include "dcr_boost.h"
#include "dcr_path.h"
#include "dcr_setup.h"
#include "error.h"
#include "gl_layer.h"
#include "jni.h"
#include "rt_applet.h"
#include "rt_boot.h"
#include "rt_cfg.h"
#include "util.h"
#include "watchdog.h"
#include "zen.h"

/* ------------------------------------------------------------ the setup
 * libgameDSO.so and classes.txt from the APK when they are missing or it has
 * changed (the .setup stamps keep their names: never rename one). */
static const char *const k_libs[] = {ZEN_LIB};

const RtSetupPlan port_setup_plan = {
    .libs = k_libs,
    .nlibs = 1,
    .apk_requirement = "This port needs ZENONIA " ZEN_VERSION " (" PORT_PACKAGE ", armeabi-v7a):\n"
                       "put the APK of that version in the game folder.",
    .libs_p0 = 0,
    .libs_p1 = 900,
    .classes_p0 = 900,
    .classes_p1 = 1000,
};

const char *port_apk_help(void) {
  return "Copy the APK of your own ZENONIA (" PORT_PACKAGE " " ZEN_VERSION ")\n"
         "there, under any name ending in .apk: the game's library, pictures,\n"
         "data and sounds come from it.";
}

/* the watchdog: frames presented */
uint64_t dcr_boot_frames(void) { return dcr_gl_frames(); }

/* ------------------------------------------------------------ natives */
typedef void (*fn_v)(void *env, void *cls);
typedef void (*fn_ii)(void *env, void *cls, jint a, jint b);
typedef jint (*fn_onload)(void *vm, void *reserved);

static fn_v n_init_ref, n_init, n_render, n_pause, n_resume, n_destroy;
static fn_ii n_resize;
static uint64_t g_frames;

static void *env(void) { return g_jni_env; }
static void *cls(void) { return jni_class(ZEN_NATIVES)->obj; }

static void bind_natives(void) {
#define BIND(var, name)                                                \
  do {                                                                 \
    var = (void *)zen_native(name);                                    \
    if (!var)                                                          \
      fatal_error("The game library has no native \"" name "\": is it ZENONIA " ZEN_VERSION "?"); \
  } while (0)
  BIND(n_init_ref, "InitializeJNIGlobalRef");
  BIND(n_init, "NativeInit");
  BIND(n_resize, "NativeResize");
  BIND(n_render, "NativeRender");
  BIND(n_pause, "NativePauseClet");
  BIND(n_resume, "NativeResumeClet");
  BIND(n_destroy, "NativeDestroyClet");
#undef BIND
}

/* ------------------------------------------------------------ boot */
int port_load(const char *apk) {
  dcr_setup_from_apk(apk);
  if (zen_apk_init(apk) != 0)
    fatal_error("Could not read the game's files from %s.\n\n"
                "Is it the APK of ZENONIA (it must hold assets/ and res/)?", apk);
  if (zen_load_module() != 0)
    fatal_error("Could not load the game library from %s.\n\n"
                "It is unpacked from the APK (" PORT_ABI_DIR ") on launch: delete\n" ZEN_LIB
                " and .setup there to unpack it again.", dcr_game_root());
  return 0;
}

/* ------------------------------------------------------------ lifecycle */
/* HOME / sleep: Android's onPause and onResume (the runtime then stops and
 * restarts the game's clocks) */
void port_focus_lost(void) {
  if (n_pause) {
    zen_audio_stop_all(); /* NexusGLActivity.onPause */
    zen_audio_pause(1);
    n_pause(env(), cls());
  }
}

void port_focus_gained(void) {
  if (n_resume) {
    zen_audio_pause(0);
    n_resume(env(), cls());
  }
}

static void exit_guard(void *arg) {
  (void)arg;
  svcSleepThread(5000000000ll);
  debugPrintf("[boot] the game did not close within 5 s: ending the process\n");
  log_flush_ring();
  svcExitProcess();
}

static void exit_guard_start(void) {
  static Thread t;
  if (R_FAILED(threadCreate(&t, exit_guard, NULL, NULL, 0x4000, 0x2B, -2)) || R_FAILED(threadStart(&t)))
    debugPrintf("[boot] no exit backstop thread\n");
}

static uint32_t audio_mixes(void) { return (uint32_t)zen_audio_mixes(); }

void zen_heap_report(void); /* zen_heap.c */

static void report(void) {
  static u64 last_tick;
  static unsigned long last_presented;
  u64 tick = armGetSystemTick();
  unsigned long presented = (unsigned long)dcr_gl_frames();
  double fps = last_tick ? (double)(presented - last_presented) * 1e9 / (double)armTicksToNs(tick - last_tick) : 0.0;
  last_tick = tick;
  last_presented = presented;
  debugPrintf("[boot] %lu frames presented (%.1f fps, the game asked for %d), %lu audio mixes, %d Java objects\n",
              presented, fps, zen_fps(), zen_audio_mixes(), jni_live_objects());
  zen_heap_report();
  dcr_boost_report();
  dcr_apkcache_report();
}

static void housekeeping(int *launch_done, unsigned long *quiet_at, u64 *last_report) {
  dcr_boost_poll();
  unsigned long frames = (unsigned long)dcr_gl_frames();
  if (!*launch_done && frames > 0) {
    *launch_done = 1;
    dcr_boost_launch_end();
    debugPrintf("[boot] first frame presented\n");
    *quiet_at = frames + 180;
  }
  /* From ~3 s after the first picture the log goes to a RAM ring (util.c),
   * written out every 10 s and by the watchdog. */
  if (*quiet_at && frames >= *quiet_at) {
    *quiet_at = 0;
    log_set_quiet(1);
  }
  u64 now = armGetSystemTick();
  if (armTicksToNs(now - *last_report) >= 10000000000ull) {
    *last_report = now;
    report();
    log_flush_ring();
  }
}

/* the thread's sleep to the period the engine set (SetSpeed) */
static void pace(u64 frame_start) {
  const u64 period = 1000000000ull / (u64)zen_fps();
  u64 used = armTicksToNs(armGetSystemTick() - frame_start);
  if (used < period)
    svcSleepThread((s64)(period - used));
}

/* System.loadLibrary: the constructors (none here), then the game. */
void port_run(void) {
  zen_run_constructors();
  zen_java_init();
  dcr_watchdog_start();
  rt_watchdog_add_counter("audio mixes", audio_mixes);
  bind_natives();

  /* ---- System.loadLibrary: JNI_OnLoad ---- */
  fn_onload onload = (fn_onload)so_try_find_addr_rx(&g_mod_game, "JNI_OnLoad");
  if (onload)
    debugPrintf("[boot] JNI_OnLoad -> 0x%lx\n", (unsigned long)onload(g_jni_vm, NULL));

  /* ---- the screen, the sound, the controls ---- */
  if (zen_gfx_init() != 0)
    fatal_error("Could not set up the graphics (EGL / OpenGL ES 1): see debug.log.");
  if (zen_audio_init() != 0)
    debugPrintf("[boot] no sound (audout did not start)\n");
  zen_input_init();

  /* ---- Activity.onResume, then the GL thread's surfaceCreated / surfaceChanged ---- */
  n_resume(env(), cls());
  n_init_ref(env(), cls());
  n_init(env(), cls());
  int w, h;
  zen_gfx_size(&w, &h);
  n_resize(env(), cls(), w, h);
  debugPrintf("[boot] up (%dx%d); this thread draws the frames now\n", w, h);
  log_flush_ring();

  u64 last_report = armGetSystemTick();
  int launch_done = 0;
  unsigned long quiet_at = 0;
  while (!rt_exit_requested() && appletMainLoop()) {
    rt_applet_poll();
    if (!rt_focused()) {
      svcSleepThread(50000000ll);
      continue;
    }
    u64 t0 = armGetSystemTick();
    zen_input_poll();
    zen_input_frame();
    n_render(env(), cls());
    zen_gfx_present();
    g_frames++;
    housekeeping(&launch_done, &quiet_at, &last_report);
    pace(t0);
  }

  /* ---- the way out: onPause, onDestroy ---- */
  log_set_quiet(0);
  rt_applet_stop(); /* the clocks run again if the game was in the background */
  exit_guard_start();
  zen_audio_stop_all();
  n_pause(env(), cls());
  n_destroy(env(), cls());
  debugPrintf("[boot] the game has closed after %llu frames\n", (unsigned long long)g_frames);
  log_flush_ring();
}
