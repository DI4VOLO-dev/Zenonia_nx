/* zen_gfx.c -- the screen: EGL and an OpenGL ES 1 context.
 *
 * On Android the engine's GL thread (NexusGLThread + EglHelper) made the
 * EGL display, config, window surface and context, and swapped after every
 * NativeRender. The engine itself only calls gl* functions (22 of them, ES
 * 1.x; the loader resolves them through the GL layer), so what is needed
 * here is that thread's EGL part: an ES 1 context on the console's window,
 * current on the thread that calls the engine, and the present. The window is
 * the runtime's (config.ini [display] resolution). MIT.
 */
#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "gl_layer.h"
#include "util.h"
#include "zen.h"

void dcr_window_prepare(void);        /* android_ndk.c */
void dcr_window_size(int *w, int *h); /* android_ndk.c */

static int g_w = 1280, g_h = 720;
void zen_gfx_size(int *w, int *h) {
  if (w)
    *w = g_w;
  if (h)
    *h = g_h;
}

#if DCR_GL_MESA
#include <EGL/egl.h>
#include <EGL/eglext.h>

static EGLDisplay g_dpy = EGL_NO_DISPLAY;
static EGLSurface g_win = EGL_NO_SURFACE;
static EGLContext g_ctx = EGL_NO_CONTEXT;
EGLBoolean b_eglSwapBuffers(EGLDisplay d, EGLSurface s); /* gl_mesa.c: frame count, hooks */

int zen_gfx_init(void) {
  if (log_console_active())
    debugPrintf("[gfx] handing the screen from the boot log to the game\n");
  log_console_close(); /* for good: see util.c */
  dcr_window_prepare();
  g_dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  EGLint maj = 0, min = 0;
  if (!eglInitialize(g_dpy, &maj, &min)) {
    debugPrintf("[gfx] eglInitialize failed 0x%x\n", (unsigned)eglGetError());
    return -1;
  }
  eglBindAPI(EGL_OPENGL_ES_API);
  /* NexusGLRenderer.getConfigSpec: {0x3025 EGL_DEPTH_SIZE, 16, 0x3038 EGL_NONE} -- a 16-bit depth buffer */
  static const EGLint want[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                                EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
                                EGL_DEPTH_SIZE, 16, EGL_NONE};
  static const EGLint plain[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT, EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
                                 EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE};
  static const EGLint any[] = {EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT, EGL_NONE};
  EGLConfig cfg;
  EGLint n = 0;
  if ((!eglChooseConfig(g_dpy, want, &cfg, 1, &n) || n < 1) && (!eglChooseConfig(g_dpy, plain, &cfg, 1, &n) || n < 1) &&
      (!eglChooseConfig(g_dpy, any, &cfg, 1, &n) || n < 1)) {
    debugPrintf("[gfx] no ES 1 config (0x%x)\n", (unsigned)eglGetError());
    return -1;
  }
  g_win = eglCreateWindowSurface(g_dpy, cfg, (EGLNativeWindowType)nwindowGetDefault(), NULL);
  static const EGLint ctx_attrs[] = {EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE};
  g_ctx = eglCreateContext(g_dpy, cfg, EGL_NO_CONTEXT, ctx_attrs);
  if (g_win == EGL_NO_SURFACE || g_ctx == EGL_NO_CONTEXT || !eglMakeCurrent(g_dpy, g_win, g_win, g_ctx)) {
    debugPrintf("[gfx] could not make an ES 1 context current (surface %p, context %p, error 0x%x)\n", g_win, g_ctx,
                (unsigned)eglGetError());
    return -1;
  }
  eglSwapInterval(g_dpy, 1);
  dcr_window_size(&g_w, &g_h);
  debugPrintf("[gfx] EGL %d.%d, an OpenGL ES 1 context, window %dx%d\n", (int)maj, (int)min, g_w, g_h);
  return 0;
}

void zen_gfx_present(void) { b_eglSwapBuffers(g_dpy, g_win); }

#else /* the null renderer: the game's code runs, nothing is drawn */
int zen_gfx_init(void) {
  dcr_window_size(&g_w, &g_h);
  return 0;
}
void zen_gfx_present(void) {}
#endif
