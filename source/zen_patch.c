/* zen_patch.c -- the touch-screen controls the game draws itself.
 *
 * ZENONIA 2 and 3 build their on-screen pad natively (not as Java views): a
 * GVUIPlayerController (InitialPlayerPadSet) adds exactly two objects, a
 * GVUIDirectionPad (the analog stick) and a GVUIBatterButton (the attack
 * button), draws them (GVUIPlayerController::Draw, GVUIController::Draw) and
 * hit-tests touches on them (GVUIController::PointerPress); every one of those
 * asks GVUIObject::IsShow() first, through the library's own PLT.
 *
 * The same GVUI objects are also the game's menus: in ZENONIA 3 the title
 * screen's "touch the screen", its background and the main menu's buttons
 * (COMMUNITY, OPTIONS, NEWGAME, CONTINUE...) are plain GVUIButton / GVUIObject
 * ones. Version 1.0.0 made IsShow answer "not shown" for every object and
 * blanked those screens. So IsShow now goes to zen_gvui_is_show below, which
 * answers "not shown" only for the two pad classes, found by their vtable
 * pointer (_ZTV + 8: an object's first word), and does what the original did
 * for every other object. With a controller the two pad objects only get in
 * the way: nothing is drawn, and a touch where they were does nothing. The
 * touch screen still reaches the game's own menus and NPCs through
 * handleCletEvent (zen_input.c). config.ini [display] touch_buttons = true
 * leaves the game's pad as it is.
 *
 * The original IsShow (4-aligned, 20 bytes) is replaced by a Thumb-2
 *     ldr.w pc, [pc, #0]     (the next word is the address to go to)
 * and the patch is checked against the instructions it replaces (sub sp, #4;
 * str r0, [sp]) and skipped, with a line in debug.log, if the library is not
 * the one it was written for. MIT.
 */
#include <stdint.h>
#include <string.h>

#include "so_util.h"
#include "util.h"
#include "zen.h"

static uintptr_t g_vt_dpad;   /* the vptr of a GVUIDirectionPad */
static uintptr_t g_vt_batter; /* the vptr of a GVUIBatterButton */

/* GVUIObject::IsShow(this): the byte at +0x30 is "hidden" (Hide sets 1, Show 0);
 * the original returned 1 & ~hidden. A C++ member function with the AAPCS
 * calling convention: this in r0, the bool in r0. */
int zen_gvui_is_show(const uint8_t *self) {
  uintptr_t vt = *(const uintptr_t *)self;
  if (vt == g_vt_dpad || vt == g_vt_batter)
    return 0;
  return (self[0x30] & 1) ? 0 : 1;
}

int zen_hide_touch_ui(void) {
  uintptr_t a = so_try_find_addr_rx(&g_mod_game, "_ZN10GVUIObject6IsShowEv");
  uintptr_t vd = so_try_find_addr_rx(&g_mod_game, "_ZTV16GVUIDirectionPad");
  uintptr_t vb = so_try_find_addr_rx(&g_mod_game, "_ZTV16GVUIBatterButton");
  if (!a || !vd || !vb) {
    debugPrintf("[patch] GVUIObject::IsShow / the pad vtables not found: the touch controls stay\n");
    return -1;
  }
  uint16_t *code = (uint16_t *)(a & ~(uintptr_t)1); /* a Thumb function: bit 0 is the state */
  if (code[0] != 0xb081 || code[1] != 0x9000 || ((uintptr_t)code & 3) != 0) {
    debugPrintf("[patch] GVUIObject::IsShow starts %04x %04x at %p, not what this was written for: the touch controls stay\n",
                code[0], code[1], (void *)code);
    return -1;
  }
  g_vt_dpad = vd + 8;
  g_vt_batter = vb + 8;
  uintptr_t target = (uintptr_t)zen_gvui_is_show; /* bit 0 is already right for its own state */
  uint8_t patch[8] = {0xdf, 0xf8, 0x00, 0xf0, /* ldr.w pc, [pc, #0] */
                      (uint8_t)target, (uint8_t)(target >> 8), (uint8_t)(target >> 16), (uint8_t)(target >> 24)};
  if (so_patch_code(code, patch, sizeof patch) != 0) {
    debugPrintf("[patch] could not write into the library: the touch controls stay\n");
    return -1;
  }
  debugPrintf("[patch] the game's on-screen pad is hidden (IsShow at %p -> %p; pad vptrs %p, %p)\n", (void *)code,
              (void *)target, (void *)g_vt_dpad, (void *)g_vt_batter);
  return 0;
}
