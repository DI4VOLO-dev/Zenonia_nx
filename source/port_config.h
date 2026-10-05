/* port_config.h -- ZENONIA's settings for the android32 runtime.
 *
 * Macros only: the runtime's C files, its assembly and the launcher all read
 * this (runtime/source/rt_settings.h). What each setting does is next to its
 * default in the runtime; runtime/docs/ lists them all. MIT.
 */
#ifndef PORT_CONFIG_H
#define PORT_CONFIG_H

/* ------------------------------------------------------------------ the game */
#define PORT_TITLE    "ZENONIA"
#define PORT_NAME     "zenonia_nx"
#define PORT_PACKAGE  "com.gamevil.zenonia"
#define PORT_BANNER   "zenonia_nx: ZENONIA (Gamevil engine, armeabi-v7a)"
#define PORT_ABI_DIR  "lib/armeabi-v7a/"
/* libgameDSO.so 1.6.6: highest p_vaddr + p_memsz = 0x1b7325 (~1.7 MB), no
 * init_array; 4 MB of reservation is plenty */
#define PORT_SO_REGION_BYTES (4u * 1024 * 1024)

/* The APK: any *.apk that is ZENONIA's (by its manifest), game.apk first,
 * else the newest -- the runtime's default role. */
#define PORT_APK_DESC "ZENONIA 1.6.6 (com.gamevil.zenonia)"
#define PORT_LAUNCHER_START_NOTE "(the first start unpacks the game's library from the APK)"

/* ------------------------------------------------------------------ libc */
#define RT_PROC_COMM "com.gamevil.zen" /* /proc/self/stat's comm (15 chars at most) */

/* ------------------------------------------------------------------ input */
#define RT_PAD_MAX_PLAYERS 1

#endif
