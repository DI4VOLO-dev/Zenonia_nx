/* zen_patch.c -- the touch-screen controls the game draws itself.
 *
 * ZENONIA 2 and 3 build their on-screen pad natively (not as Java views): a
 * GVUIPlayerController (InitialPlayerPadSet) adds exactly two objects, a
 * GVUIDirectionPad (the analog stick) and a GVUIBatterButton (the attack
 * button), draws them (GVUIPlayerController::Draw, GVUIController::Draw) and
 * hit-tests touches on them (GVUIController::PointerPress); every one of those
 * three asks GVUIObject::IsShow() first, through the library's own PLT.
 *
 * With a controller in hand they only get in the way, so IsShow is made to
 * answer "not shown" (movs r0, #0; bx lr): nothing is drawn and a touch on
 * where they were does nothing. The touch screen still reaches the game's own
 * menus and NPCs through handleCletEvent (zen_input.c). config.ini [display]
 * touch_buttons = true leaves the game's pad as it is.
 *
 * The patch is checked against the instructions it replaces (sub sp, #4; str
 * r0, [sp]) and skipped, with a line in debug.log, if the library is not the
 * one it was written for. MIT.
 */
#include <stdint.h>
#include <string.h>

#include "so_util.h"
#include "util.h"
#include "zen.h"

int zen_hide_touch_ui(void) {
  uintptr_t a = so_try_find_addr_rx(&g_mod_game, "_ZN10GVUIObject6IsShowEv");
  if (!a) {
    debugPrintf("[patch] GVUIObject::IsShow not found: the touch controls stay\n");
    return -1;
  }
  uint16_t *code = (uint16_t *)(a & ~(uintptr_t)1); /* a Thumb function: bit 0 is the state */
  if (code[0] != 0xb081 || code[1] != 0x9000) {
    debugPrintf("[patch] GVUIObject::IsShow starts %04x %04x, not what this was written for: the touch controls stay\n",
                code[0], code[1]);
    return -1;
  }
  static const uint8_t patch[4] = {0x00, 0x20, 0x70, 0x47}; /* movs r0, #0 ; bx lr */
  if (so_patch_code(code, patch, sizeof patch) != 0) {
    debugPrintf("[patch] could not write into the library: the touch controls stay\n");
    return -1;
  }
  debugPrintf("[patch] the game's on-screen pad is hidden (GVUIObject::IsShow at %p)\n", (void *)code);
  return 0;
}
