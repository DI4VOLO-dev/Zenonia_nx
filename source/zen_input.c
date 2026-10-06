/* zen_input.c -- the controller and the touch screen, as ZENONIA 2 and 3's Java
 * gave them to the engine.
 *
 * There is no setInputEvent in this version: every input is an event that
 * NexusGLRenderer.setTouchEvent(type, p1, p2, pointerId) put in a queue, and
 * that drawFrame takes out ONE per frame, just before NativeRender, as
 * Natives.handleCletEvent(type, p1, p2, pointerId):
 *   type 2   key press   p1 = the engine's key code (UIControllerView.getHalKeyCode)
 *   type 3   key release
 *   type 23  touch down  p1, p2 = x, y in the game's 400 x 240 screen
 *   type 25  touch move                       (x * 400 / screen width,
 *   type 24  touch up                          y * 240 / screen height)
 * The key codes the Java made out of Android's keys:
 *   -1 -2 -3 -4   up, down, left, right       (DPAD_UP, DOWN, LEFT, RIGHT)
 *   -5            OK / attack                 (DPAD_CENTER, ENTER)
 *   -8            back / cancel               (BACK)
 *   -16           the game's menu             (DEL, CLEAR, BUTTON_START, BUTTON_Y)
 *   35  '#'       skill / slot swap           (A key, BUTTON_X)
 *   17  '*'       map                         (STAR; Q, W, E, BUTTON_SELECT)
 *   49 51 55 57   the quick slots, '1' '3' '7' '9' (the number keys: 48 + digit)
 * The directions are separate keys, so two held at once are a diagonal.
 * The queue is dropped into one event per frame like the Java's (a press and
 * its release arrive in two frames), but two when it has backed up. MIT.
 */
#include <math.h>
#include <string.h>
#include <switch.h>

#include "rt_pad.h"
#include "util.h"
#include "zen.h"
#include "zen_config.h"

#define ZEV_KEY_DOWN 2
#define ZEV_KEY_UP 3
#define ZEV_TOUCH_DOWN 23
#define ZEV_TOUCH_UP 24
#define ZEV_TOUCH_MOVE 25

#define K_UP (-1)
#define K_DOWN (-2)
#define K_LEFT (-3)
#define K_RIGHT (-4)

typedef void (*fn_iiii)(void *env, void *cls, jint a, jint b, jint c, jint d);
static fn_iiii g_handle_clet;
static PadState g_pad[2]; /* player 1, and the Joy-Cons on the console */
static int g_have_touch;

/* ------------------------------------------------------------------ the queue */
typedef struct {
  int type, p1, p2, pid;
} Ev;
#define QCAP 96
static Ev g_q[QCAP];
static int g_qhead, g_qn;

static void *env(void) { return g_jni_env; }
static void *cls(void) { return jni_class(ZEN_NATIVES)->obj; }

static void enqueue(int type, int p1, int p2) {
  if (g_qn == QCAP) {
    if (type == ZEV_TOUCH_MOVE)
      return; /* a move is the one event nobody misses */
    g_qhead = (g_qhead + 1) % QCAP; /* the oldest gives way */
    g_qn--;
  }
  /* a move right after a move: the newer one replaces it */
  if (type == ZEV_TOUCH_MOVE && g_qn > 0) {
    Ev *last = &g_q[(g_qhead + g_qn - 1) % QCAP];
    if (last->type == ZEV_TOUCH_MOVE) {
      last->p1 = p1;
      last->p2 = p2;
      return;
    }
  }
  Ev *e = &g_q[(g_qhead + g_qn) % QCAP];
  e->type = type;
  e->p1 = p1;
  e->p2 = p2;
  e->pid = 0;
  g_qn++;
}

/* an event now, outside the queue (the pause events) */
void zen_clet_event(int type, int p1, int p2, int pid) {
  if (g_handle_clet)
    g_handle_clet(env(), cls(), type, p1, p2, pid);
}

static void key(int down, int code) { enqueue(down ? ZEV_KEY_DOWN : ZEV_KEY_UP, code, 0); }

void zen_input_init(void) {
  g_handle_clet = (fn_iiii)zen_native("handleCletEvent");
  if (!g_handle_clet)
    debugPrintf("[input] MISSING native handleCletEvent\n");
  rt_pad_setup(1, 1);
  rt_pad_slot(&g_pad[0], 0);
  rt_pad_slot(&g_pad[1], RT_PAD_HANDHELD);
  hidInitializeTouchScreen();
  g_have_touch = 1;
}

/* a button: its mask, the engine's key code, whether it is down */
typedef struct {
  u64 mask;
  int code;
  int down;
} Btn;

static Btn g_btns[] = {
    {HidNpadButton_A, -5, 0},       {HidNpadButton_B, -16, 0},      {HidNpadButton_X, 17, 0},
    {HidNpadButton_Y, 35, 0},       {HidNpadButton_ZL, 49, 0},      {HidNpadButton_ZR, 51, 0},
    {HidNpadButton_StickL, 55, 0},  {HidNpadButton_StickR, 57, 0},  {HidNpadButton_Plus, -16, 0},
    {HidNpadButton_Minus, -8, 0},   {HidNpadButton_L, -8, 0},       {HidNpadButton_R, 35, 0},
};

static int g_dir[4]; /* held: up, down, left, right */
static const int k_dir_code[4] = {K_UP, K_DOWN, K_LEFT, K_RIGHT};

static int g_touch_down;
static int g_touch_x = -1, g_touch_y = -1;

void zen_input_poll(void) {
  float sticks[4] = {0, 0, 0, 0}, s1[4];
  u64 buttons = 0;
  for (int i = 0; i < 2; i++) {
    padUpdate(&g_pad[i]);
    buttons |= rt_pad_read(&g_pad[i], i == 0 ? sticks : s1);
  }
  if (zen_config()->swap_ab) {
    u64 a = buttons & HidNpadButton_A, b = buttons & HidNpadButton_B;
    buttons &= ~(u64)(HidNpadButton_A | HidNpadButton_B);
    buttons |= (a ? HidNpadButton_B : 0) | (b ? HidNpadButton_A : 0);
  }
  for (unsigned i = 0; i < sizeof g_btns / sizeof g_btns[0]; i++) {
    int down = (buttons & g_btns[i].mask) != 0;
    if (down == g_btns[i].down)
      continue;
    g_btns[i].down = down;
    key(down, g_btns[i].code);
  }
  /* the directions: the D-pad and the left stick, each axis on its own (two
   * axes at once are a diagonal, as two keys held would be) */
  int want[4];
  want[0] = (buttons & HidNpadButton_Up) || sticks[1] > 0.45f;
  want[1] = (buttons & HidNpadButton_Down) || sticks[1] < -0.45f;
  want[2] = (buttons & HidNpadButton_Left) || sticks[0] < -0.45f;
  want[3] = (buttons & HidNpadButton_Right) || sticks[0] > 0.45f;
  if (want[0] && want[1])
    want[0] = want[1] = 0;
  if (want[2] && want[3])
    want[2] = want[3] = 0;
  for (int d = 0; d < 4; d++) {
    if (want[d] != g_dir[d]) {
      g_dir[d] = want[d];
      key(want[d], k_dir_code[d]);
    }
  }
  /* the touch screen: the game's own coordinates */
  if (g_have_touch) {
    HidTouchScreenState ts;
    int down = hidGetTouchScreenStates(&ts, 1) > 0 && ts.count > 0;
    int w = 1280, h = 720;
    zen_gfx_size(&w, &h);
    if (down) {
      int x = (int)ts.touches[0].x * 400 / (w > 0 ? w : 1280);
      int y = (int)ts.touches[0].y * 240 / (h > 0 ? h : 720);
      if (!g_touch_down) {
        enqueue(ZEV_TOUCH_DOWN, x, y);
      } else if (x != g_touch_x || y != g_touch_y) {
        enqueue(ZEV_TOUCH_MOVE, x, y);
      }
      g_touch_x = x;
      g_touch_y = y;
    } else if (g_touch_down) {
      enqueue(ZEV_TOUCH_UP, g_touch_x, g_touch_y);
    }
    g_touch_down = down;
  }
}

/* NexusGLRenderer.sendHandleCletEvent: one event, before NativeRender */
void zen_input_frame(void) {
  int n = g_qn > 4 ? 2 : 1;
  while (n-- > 0 && g_qn > 0) {
    Ev e = g_q[g_qhead];
    g_qhead = (g_qhead + 1) % QCAP;
    g_qn--;
    zen_clet_event(e.type, e.p1, e.p2, e.pid);
  }
}
