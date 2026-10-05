/* zen_config.c -- <game folder>/config.ini, on the runtime's INI engine
 * (runtime/source/rt_cfg.c): written with every option, its default and a
 * line of explanation on the first start; an existing file is appended to, so
 * a player's edits survive updates. Read once at start-up. MIT.
 */
#include <stdio.h>
#include <string.h>

#include "rt_cfg.h"
#include "util.h"
#include "zen_config.h"

static ZenConfig g_cfg = {.smooth = 1, .touch_buttons = 0, .volume = 100, .swap_ab = 0};

const ZenConfig *zen_config(void) { return &g_cfg; }

static const CfgOpt k_opts[] = {
    CFG_ROW_RESOLUTION("720", CFG_HELP_RESOLUTION),
    {"display", "smooth", "true",
     "Smooth (bilinear) scaling of the game's picture. false: sharp pixels.", CFG_BOOL, NULL, &g_cfg.smooth, 0, 0, 0, 0},
    {"display", "touch_buttons", "false",
     "Draw the game's own on-screen D-pad and buttons (made for a phone's touch\n"
     "# screen). Off: the controller does it all.",
     CFG_BOOL, NULL, &g_cfg.touch_buttons, 0, 0, 0, 0},
    {"audio", "volume", "100", "Sound volume, 0 to 100.", CFG_INT, NULL, &g_cfg.volume, 0, 100, 0, 0},
    CFG_ROW_SWAP_AB("Swap A and B (true: A is the menu / cancel button, B attacks).", &g_cfg.swap_ab),
    CFG_ROW_BOOST(CFG_HELP_BOOST, NULL),
    CFG_ROW_GL_SELFTEST(NULL),
    CFG_ROW_BOOT_LOG(CFG_HELP_BOOT_LOG, NULL),
    CFG_ROW_LOG_JNI(CFG_HELP_LOG_JNI, NULL),
};

static void apply(void) {
  const RtConfig *rt = rt_config();
  debugPrintf("[config] %dx%d, smooth %s, touch buttons %s, volume %d, A/B %s\n", rt->res_w, rt->res_h,
              g_cfg.smooth ? "on" : "off", g_cfg.touch_buttons ? "shown" : "hidden", g_cfg.volume,
              g_cfg.swap_ab ? "swapped" : "normal");
}

static const CfgTable k_table = {
    .opts = k_opts,
    .nopts = CFG_COUNT(k_opts),
    .version = 1,
    .apply = apply,
};

void dcr_config_load(void) { rt_config_load(&k_table); }
