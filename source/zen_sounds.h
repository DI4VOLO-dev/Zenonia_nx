/* zen_sounds.h -- the sound table of ZENONIA 3 (com.gamevil.zenonia3.global.skt
 * 1.0.3): the res/raw file behind each sound number. Read from the APK's own
 * resource table (the names are an interface, not game content). MIT.
 *
 * Unlike ZENONIA 2 there is no translation table: the Java adds the sound
 * number to the first raw resource id (0x7f050000 + n), and the 15 clips of the
 * SoundPool (numbers 1..15, NexusSound.playSFXSound) are the first 15 of the
 * same list. All 76 files are MP3 (22.05 kHz mono and 44.1 kHz mono/stereo). */
#ifndef ZEN_SOUNDS_H
#define ZEN_SOUNDS_H

static const char *const k_zen_files[] = {
    "res/Db9.mp3", /* 0 */
    "res/Db8.mp3", /* 1 */
    "res/Db7.mp3", /* 2 */
    "res/Db6.mp3", /* 3 */
    "res/Db5.mp3", /* 4 */
    "res/Db4.mp3", /* 5 */
    "res/Db3.mp3", /* 6 */
    "res/Db2.mp3", /* 7 */
    "res/Db1.mp3", /* 8 */
    "res/Db.mp3", /* 9 */
    "res/z67.mp3", /* 10 */
    "res/z66.mp3", /* 11 */
    "res/z65.mp3", /* 12 */
    "res/z64.mp3", /* 13 */
    "res/z63.mp3", /* 14 */
    "res/z62.mp3", /* 15 */
    "res/z61.mp3", /* 16 */
    "res/z6.mp3", /* 17 */
    "res/ja8.mp3", /* 18 */
    "res/ja7.mp3", /* 19 */
    "res/ja6.mp3", /* 20 */
    "res/ja5.mp3", /* 21 */
    "res/ja4.mp3", /* 22 */
    "res/ja3.mp3", /* 23 */
    "res/ja2.mp3", /* 24 */
    "res/ja1.mp3", /* 25 */
    "res/ja.mp3", /* 26 */
    "res/T28.mp3", /* 27 */
    "res/T27.mp3", /* 28 */
    "res/T26.mp3", /* 29 */
    "res/T25.mp3", /* 30 */
    "res/T24.mp3", /* 31 */
    "res/T23.mp3", /* 32 */
    "res/T22.mp3", /* 33 */
    "res/T21.mp3", /* 34 */
    "res/T2.mp3", /* 35 */
    "res/DW4.mp3", /* 36 */
    "res/DW3.mp3", /* 37 */
    "res/DW2.mp3", /* 38 */
    "res/DW1.mp3", /* 39 */
    "res/DW.mp3", /* 40 */
    "res/z14.mp3", /* 41 */
    "res/z13.mp3", /* 42 */
    "res/z12.mp3", /* 43 */
    "res/z11.mp3", /* 44 */
    "res/z1.mp3", /* 45 */
    "res/jV4.mp3", /* 46 */
    "res/jV3.mp3", /* 47 */
    "res/jV2.mp3", /* 48 */
    "res/jV1.mp3", /* 49 */
    "res/jV.mp3", /* 50 */
    "res/T14.mp3", /* 51 */
    "res/T13.mp3", /* 52 */
    "res/T12.mp3", /* 53 */
    "res/T11.mp3", /* 54 */
    "res/T1.mp3", /* 55 */
    "res/DV1.mp3", /* 56 */
    "res/DV.mp3", /* 57 */
    "res/z01.mp3", /* 58 */
    "res/z0.mp3", /* 59 */
    "res/LZ9.mp3", /* 60 */
    "res/LZ8.mp3", /* 61 */
    "res/LZ7.mp3", /* 62 */
    "res/LZ6.mp3", /* 63 */
    "res/LZ5.mp3", /* 64 */
    "res/LZ4.mp3", /* 65 */
    "res/LZ3.mp3", /* 66 */
    "res/LZ2.mp3", /* 67 */
    "res/LZ1.mp3", /* 68 */
    "res/LZ.mp3", /* 69 */
    "res/bZ5.mp3", /* 70 */
    "res/bZ4.mp3", /* 71 */
    "res/bZ3.mp3", /* 72 */
    "res/bZ2.mp3", /* 73 */
    "res/bZ1.mp3", /* 74 */
    "res/bZ.mp3", /* 75 */
};
#define ZEN_NFILES ((int)(sizeof k_zen_files / sizeof k_zen_files[0]))

#endif
