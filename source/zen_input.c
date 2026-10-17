/* zen_input.c -- the controller (and the touch screen) as the game's Java views
 * gave it input.
 *
 * Every view of the phone version ended in one of two calls:
 *   NexusGLRenderer.setTouchEvent(type, code, 0): Natives.setInputEvent(...) at
 *       once, and the same event to Natives.handleCletEvent(...) at the start
 *       of the next frame (drawFrame: only the last event of a frame; the
 *       renderer resets it to type 8, "none");
 *   Natives.setInputEvent(type, code, 0) alone (the two menu-tab buttons).
 * type 2 is a press, 3 a release. The codes are J2ME's key codes, as the
 * on-screen buttons sent them:
 *   directions   -1 up, -2 down, -3 left, -4 right   (UIDirectionPad.getDirection)
 *   -5  select / attack / OK        (UISelectButton, the dialogs' OK)
 *   -6  / -7  left / right soft key (UIMenuLeft / UIMenuRight: the menu's tabs)
 *   -8  "any touch" / back          (UIAnyTouch; KEYCODE_BACK; skips text)
 *   -16 game menu / cancel          (UIGameMenuButton, UICancelButton)
 *   42  '*' map                     (UIMapButton)
 *   35  '#' slot swap / skip        (UIQuickSlotSwapButton, UISkipButton)
 *   49 51 55 57  the four quick slots, '1' '3' '7' '9'  (UIQuickSlot)
 * The directions' order (-1 up ... -4 right) is the J2ME one; if the hero
 * walks the wrong way, the four DIR_ constants below are the place. MIT.
 */
#include <math.h>
#include <string.h>
#include <switch.h>

#include "rt_pad.h"
#include "util.h"
#include "zen.h"
#include "zen_config.h"

#define ZEV_PRESS 2
#define ZEV_RELEASE 3
#define ZEV_NONE 8

#define DIR_UP (-1)
#define DIR_DOWN (-2)
#define DIR_LEFT (-3)
#define DIR_RIGHT (-4)

typedef void (*fn_iii)(void *env, void *cls, jint a, jint b, jint c);
static fn_iii g_set_input, g_handle_clet;
static PadState g_pad[2]; /* player 1, and the Joy-Cons on the console */
static int g_have_touch;

/* the renderer's pending event */
static int g_cur_type = ZEV_NONE, g_cur_p1, g_cur_p2;

static void *env(void) { return g_jni_env; }
static void *cls(void) { return jni_class(ZEN_NATIVES)->obj; }

/* direct: Natives.setInputEvent only (the menu tabs) */
static void send(int type, int code, int direct) {
  debugPrintf("[input] type %d code %d%s\n", type, code, direct ? " (direct)" : "");
  if (g_set_input)
    g_set_input(env(), cls(), type, code, 0);
  if (!direct) {
    g_cur_type = type;
    g_cur_p1 = code;
    g_cur_p2 = 0;
  }
}

void zen_input_init(void) {
  g_set_input = (fn_iii)zen_native("setInputEvent");
  g_handle_clet = (fn_iii)zen_native("handleCletEvent");
  if (!g_set_input || !g_handle_clet)
    debugPrintf("[input] MISSING native setInputEvent / handleCletEvent\n");
  rt_pad_setup(1, 1);
  rt_pad_slot(&g_pad[0], 0);
  rt_pad_slot(&g_pad[1], RT_PAD_HANDHELD);
  hidInitializeTouchScreen();
  g_have_touch = 1;
}

/* a held input: its code, whether it is down, and how it goes out */
typedef struct {
  u64 mask;
  int code;
  int direct;
  int down;
} Key;

static Key g_keys[] = {
    {HidNpadButton_A, -5, 0, 0},       {HidNpadButton_B, -16, 0, 0},      {HidNpadButton_X, 42, 0, 0},
    {HidNpadButton_Y, 35, 0, 0},       {HidNpadButton_L, -6, 1, 0},       {HidNpadButton_R, -7, 1, 0},
    {HidNpadButton_ZL, 49, 0, 0},      {HidNpadButton_ZR, 51, 0, 0},      {HidNpadButton_StickL, 55, 0, 0},
    {HidNpadButton_StickR, 57, 0, 0},  {HidNpadButton_Plus, -16, 0, 0},   {HidNpadButton_Minus, -8, 0, 0},
};

static int g_dir; /* the direction held now (a DIR_ code), 0 none */
static int g_touch_down;

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
  for (unsigned i = 0; i < sizeof g_keys / sizeof g_keys[0]; i++) {
    int down = (buttons & g_keys[i].mask) != 0;
    if (down == g_keys[i].down)
      continue;
    g_keys[i].down = down;
    send(down ? ZEV_PRESS : ZEV_RELEASE, g_keys[i].code, g_keys[i].direct);
  }
  /* the four directions: the D-pad, else the left stick's stronger axis (the
   * phone's pad was 4-way) */
  int want = 0;
  if (buttons & HidNpadButton_Up)
    want = DIR_UP;
  else if (buttons & HidNpadButton_Down)
    want = DIR_DOWN;
  else if (buttons & HidNpadButton_Left)
    want = DIR_LEFT;
  else if (buttons & HidNpadButton_Right)
    want = DIR_RIGHT;
  else if (fabsf(sticks[0]) > 0.45f || fabsf(sticks[1]) > 0.45f) {
    if (fabsf(sticks[0]) > fabsf(sticks[1]))
      want = sticks[0] < 0 ? DIR_LEFT : DIR_RIGHT;
    else
      want = sticks[1] > 0 ? DIR_UP : DIR_DOWN;
  }
  if (want != g_dir) {
    if (g_dir)
      send(ZEV_RELEASE, g_dir, 0);
    if (want)
      send(ZEV_PRESS, want, 0);
    g_dir = want;
  }
  /* a tap anywhere is UIAnyTouch: -8 (text, pictures) */
  if (g_have_touch) {
    HidTouchScreenState ts;
    int down = hidGetTouchScreenStates(&ts, 1) > 0 && ts.count > 0;
    if (down != g_touch_down) {
      g_touch_down = down;
      send(down ? ZEV_PRESS : ZEV_RELEASE, -8, 0);
    }
  }
}

/* NexusGLRenderer.sendHandleCletEvent, before NativeRender */
void zen_input_frame(void) {
  if (g_cur_type == ZEV_NONE)
    return;
  if (g_handle_clet)
    g_handle_clet(env(), cls(), g_cur_type, g_cur_p1, g_cur_p2);
  g_cur_type = ZEV_NONE;
  g_cur_p1 = g_cur_p2 = 0;
}
