/* zen_loader.c -- loading ZENONIA's engine.
 *
 * The APK carries one library, lib/armeabi-v7a/libgameDSO.so: Gamevil's
 * "Nexus" engine and the game (ARMv7, Thumb-2, NEON). It imports bionic's
 * libc/libm, libstdc++'s guards, liblog, 22 OpenGL ES 1 functions (resolved
 * by the loader through the GL layer) and a few sockets; it has no
 * init_array, a JNI_OnLoad and 21 native entry points, all in the class
 * com.gamevil.nexus2.Natives. Nothing in it writes code at run time, so the
 * module is mapped and sealed in one go. MIT.
 */
#include <stdio.h>
#include <string.h>
#include <switch.h>

#include "config.h"
#include "dcr_path.h"
#include "error.h"
#include "imports.h"
#include "so_util.h"
#include "util.h"
#include "zen.h"

so_module g_mod_game;

void *zen_native(const char *name) {
  char sym[160];
  snprintf(sym, sizeof sym, "Java_com_gamevil_nexus2_Natives_%s", name);
  return (void *)so_try_find_addr_rx(&g_mod_game, sym);
}

int zen_load_module(void) {
  char path[512];
  snprintf(path, sizeof path, "%s/%s", dcr_game_root(), ZEN_LIB);
  so_module *m = &g_mod_game;
  int rc = so_load(m, path, NULL, PORT_SO_REGION_BYTES);
  if (rc < 0) {
    const char *why = rc == -1 ? "cannot open it, or it is not a 32-bit ARM ELF"
                    : rc == -2 ? "out of memory"
                    : rc == -3 ? "larger than PORT_SO_REGION_BYTES"
                    : rc == -4 ? "too many program headers" : "?";
    debugPrintf("[boot] so_load(%s) failed rc=%d: %s\n", path, rc, why);
    return -1;
  }
  so_relocate(m);
  int missing = so_resolve(m, dcr_imports, dcr_imports_count, 1);
  debugPrintf("[boot] %s %u KB staged %p -> %p (%d unresolved imports)\n", m->base_name,
              (unsigned)(m->load_size >> 10), m->load_base, m->load_virtbase, missing);
  so_finalize(m);
  so_flush_caches(m);
  return 0;
}

/* System.loadLibrary("gameDSO") runs the constructors: this library has none,
 * but the call is the runtime's usual one and costs nothing. */
void zen_run_constructors(void) {
  u64 t0 = armGetSystemTick();
  so_execute_init_array(&g_mod_game);
  debugPrintf("[boot] %s constructors done in %llu ms\n", g_mod_game.base_name,
              (unsigned long long)(armTicksToNs(armGetSystemTick() - t0) / 1000000ull));
}
