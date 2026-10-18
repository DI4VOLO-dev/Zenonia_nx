/* zen_input.c -- the controller and the touch screen, as ZENONIA 2's Java
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
 *
 * The engine keeps ONE key, not a set of them (CGsInputKey, read off the
 * library): a press replaces whatever key was held, and ANY release -- of any
 * key -- clears it (CMvApp::EvKeyRelease calls SetReleaseKey(false) without
 * looking at which key went up); the player walks in four directions from
 * that one key (CMvPlayer::OnKeyPress). So the pad must be turned into one
 * held key at a time, and when something else takes the slot (the attack
 * button's press and release) the direction has to be pressed again, or the
 * hero stops while the stick is still tilted. zen_input_poll does that from
 * a model of the slot: what the engine holds after the events already
 * delivered and those still queued.
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
static int g_eng; /* the engine's one key after the events delivered so far (0: none) */

void zen_clet_event(int type, int p1, int p2, int pid) {
  if (type == 99 || type == 9900)
    g_eng = 0; /* the pause events: the game lets go of its keys */
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

static int g_want; /* the direction the pad asked for at the last poll (0, or a K_ code) */

/* what the engine's one key will be once everything queued is delivered */
static int predicted_slot(void) {
  int slot = g_eng;
  for (int i = 0; i < g_qn; i++) {
    const Ev *e = &g_q[(g_qhead + i) % QCAP];
    if (e->type == ZEV_KEY_DOWN)
      slot = e->p1;
    else if (e->type == ZEV_KEY_UP)
      slot = 0;
  }
  return slot;
}

/* the D-pad: one direction; the one already wanted stays while it is held */
static int dpad_dir(u64 b) {
  int up = (b & HidNpadButton_Up) != 0, down = (b & HidNpadButton_Down) != 0;
  int left = (b & HidNpadButton_Left) != 0, right = (b & HidNpadButton_Right) != 0;
  if ((g_want == K_UP && up) || (g_want == K_DOWN && down) || (g_want == K_LEFT && left) || (g_want == K_RIGHT && right))
    return g_want;
  if (up)
    return K_UP;
  if (down)
    return K_DOWN;
  if (left)
    return K_LEFT;
  if (right)
    return K_RIGHT;
  return 0;
}

/* the left stick as one of four directions, with hysteresis: it takes more to
 * start (0.50) than to keep (0.30), and a direction only gives way to the
 * other axis when that one clearly leads, so a thumb resting near a diagonal
 * does not flicker between two keys */
static int stick_dir(float x, float y) {
  static int cur;
  float ax = fabsf(x), ay = fabsf(y);
  float m = ax > ay ? ax : ay;
  if (m < (cur ? 0.30f : 0.50f)) {
    cur = 0;
    return 0;
  }
  int horiz = ax >= ay;
  int cand = horiz ? (x < 0 ? K_LEFT : K_RIGHT) : (y > 0 ? K_UP : K_DOWN);
  if (cur && cand != cur) {
    int cur_h = cur == K_LEFT || cur == K_RIGHT;
    if (cur_h != horiz) { /* the other axis: only when it clearly leads */
      float cur_mag = cur_h ? ax : ay, cand_mag = horiz ? ax : ay;
      if (cand_mag < cur_mag * 1.3f + 0.05f)
        return cur;
    }
  }
  cur = cand;
  return cand;
}

static int g_touch_down;
static int g_touch_x = -1, g_touch_y = -1;

void zen_input_poll(void) {
  float sticks[4] = {0, 0, 0, 0}, s1[4] = {0, 0, 0, 0};
  u64 buttons = 0;
  for (int i = 0; i < 2; i++) {
    padUpdate(&g_pad[i]);
    buttons |= rt_pad_read(&g_pad[i], i == 0 ? sticks : s1);
  }
  /* The Joy-Cons on the console (handheld) are the second slot, and their left
   * stick used to be dropped: it counts too, whichever is pushed further wins. */
  if (fabsf(s1[0]) + fabsf(s1[1]) > fabsf(sticks[0]) + fabsf(sticks[1])) {
    sticks[0] = s1[0];
    sticks[1] = s1[1];
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
  /* the direction: the D-pad, else the left stick, as ONE held key. A press
   * replaces the key before it, so a change of direction is just a press; a
   * release is only sent when the direction key is still the engine's key
   * (the attack button's press and release leave the slot empty, and then the
   * direction is pressed again; while another button is held it is left alone). */
  int other_held = 0;
  for (unsigned i = 0; i < sizeof g_btns / sizeof g_btns[0]; i++)
    other_held |= g_btns[i].down;
  int want = dpad_dir(buttons);
  if (!want)
    want = stick_dir(sticks[0], sticks[1]);
  int slot = predicted_slot();
  if (want) {
    if (slot != want && !other_held)
      key(1, want);
  } else if (g_want && slot == g_want) {
    key(0, g_want);
  }
  g_want = want;
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
    if (e.type == ZEV_KEY_DOWN)
      g_eng = e.p1;
    else if (e.type == ZEV_KEY_UP)
      g_eng = 0;
  }
}
