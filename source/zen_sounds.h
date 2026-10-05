/* zen_sounds.h -- the sound tables of ZENONIA 2 (com.gamevil.zenonia2.global
 * 1.0.3): which of the APK's res/raw files each sound number the engine asks
 * for is. Read from the APK's own code and resource table (the numbers and file
 * names are an interface, not game content). MIT.
 *
 *  - NexusSound.playSFXSound(n), n = 1..15 (13 is played as 12): the 15 clips
 *    the game loads into a SoundPool at start-up (addSFXSound), k_zen_sfx;
 *  - every other number (0 and 16 up) goes through
 *    ZenoniaUIControllerView.getAudioResource to a raw resource, k_zen_audio:
 *    looping ones are the background music, the rest a one-at-a-time clip.
 * All 74 files are 16-bit mono PCM WAV at 22050 Hz. */
#ifndef ZEN_SOUNDS_H
#define ZEN_SOUNDS_H

static const char *const k_zen_wavs[] = {
    "res/bc9.wav", /* 0: s000 */
    "res/bc8.wav", /* 1: s001 */
    "res/bc7.wav", /* 2: s002 */
    "res/bc6.wav", /* 3: s003 */
    "res/bc5.wav", /* 4: s004 */
    "res/bc4.wav", /* 5: s005 */
    "res/bc3.wav", /* 6: s006 */
    "res/bc2.wav", /* 7: s007 */
    "res/bc1.wav", /* 8: s008 */
    "res/bc.wav", /* 9: s009 */
    "res/Lz8.wav", /* 10: s010 */
    "res/Lz7.wav", /* 11: s011 */
    "res/Lz6.wav", /* 12: s012 */
    "res/Lz5.wav", /* 13: s013 */
    "res/Lz4.wav", /* 14: s014 */
    "res/Lz3.wav", /* 15: s015 */
    "res/Lz2.wav", /* 16: s016 */
    "res/Lz1.wav", /* 17: s017 */
    "res/Lz.wav", /* 18: s018 */
    "res/7T2.wav", /* 19: s020 */
    "res/7T1.wav", /* 20: s022 */
    "res/7T.wav", /* 21: s023 */
    "res/r-6.wav", /* 22: s033 */
    "res/r-5.wav", /* 23: s034 */
    "res/r-4.wav", /* 24: s035 */
    "res/r-3.wav", /* 25: s036 */
    "res/r-2.wav", /* 26: s037 */
    "res/r-1.wav", /* 27: s038 */
    "res/r-.wav", /* 28: s039 */
    "res/bf9.wav", /* 29: s040 */
    "res/bf8.wav", /* 30: s041 */
    "res/bf7.wav", /* 31: s042 */
    "res/bf6.wav", /* 32: s043 */
    "res/bf5.wav", /* 33: s044 */
    "res/bf4.wav", /* 34: s045 */
    "res/bf3.wav", /* 35: s046 */
    "res/bf2.wav", /* 36: s047 */
    "res/bf1.wav", /* 37: s048 */
    "res/bf.wav", /* 38: s049 */
    "res/L-9.wav", /* 39: s050 */
    "res/L-8.wav", /* 40: s051 */
    "res/L-7.wav", /* 41: s052 */
    "res/L-6.wav", /* 42: s053 */
    "res/L-5.wav", /* 43: s054 */
    "res/L-4.wav", /* 44: s055 */
    "res/L-3.wav", /* 45: s056 */
    "res/L-2.wav", /* 46: s057 */
    "res/L-1.wav", /* 47: s058 */
    "res/L-.wav", /* 48: s059 */
    "res/7e8.wav", /* 49: s060 */
    "res/7e7.wav", /* 50: s061 */
    "res/7e6.wav", /* 51: s062 */
    "res/7e5.wav", /* 52: s063 */
    "res/7e4.wav", /* 53: s064 */
    "res/7e3.wav", /* 54: s065 */
    "res/7e2.wav", /* 55: s066 */
    "res/7e1.wav", /* 56: s067 */
    "res/7e.wav", /* 57: s068 */
    "res/jh7.wav", /* 58: s100 */
    "res/jh6.wav", /* 59: s101 */
    "res/jh5.wav", /* 60: s102 */
    "res/jh4.wav", /* 61: s103 */
    "res/jh3.wav", /* 62: s104 */
    "res/jh2.wav", /* 63: s105 */
    "res/jh1.wav", /* 64: s106 */
    "res/jh.wav", /* 65: s108 */
    "res/zB6.wav", /* 66: s110 */
    "res/zB5.wav", /* 67: s111 */
    "res/zB4.wav", /* 68: s113 */
    "res/zB3.wav", /* 69: s114 */
    "res/zB2.wav", /* 70: s115 */
    "res/zB1.wav", /* 71: s116 */
    "res/zB.wav", /* 72: s119 */
    "res/Dh.wav", /* 73: s120 */
};
#define ZEN_NWAVS ((int)(sizeof k_zen_wavs / sizeof k_zen_wavs[0]))

/* SoundPool keys 1..15 -> index in k_zen_wavs (-1: none) */
static const signed char k_zen_sfx[16] = {
    -1,1,2,3,4,5,6,7,8,9,11,12,13,14,15,16};

/* getAudioResource: sound number -> index in k_zen_wavs */
typedef struct { short id; short wav; } ZenAudioMap;
static const ZenAudioMap k_zen_audio[] = {
    {0, 0},
    {1, 1},
    {2, 2},
    {3, 3},
    {4, 4},
    {5, 5},
    {6, 6},
    {7, 7},
    {8, 8},
    {9, 9},
    {10, 10},
    {11, 11},
    {12, 12},
    {13, 13},
    {14, 14},
    {15, 15},
    {16, 16},
    {17, 17},
    {18, 18},
    {20, 19},
    {22, 20},
    {23, 21},
    {33, 22},
    {34, 23},
    {35, 24},
    {36, 25},
    {37, 26},
    {38, 27},
    {39, 28},
    {40, 29},
    {41, 30},
    {42, 31},
    {43, 32},
    {44, 33},
    {45, 34},
    {46, 35},
    {47, 36},
    {48, 37},
    {49, 38},
    {50, 39},
    {51, 40},
    {52, 41},
    {53, 42},
    {54, 43},
    {55, 44},
    {56, 45},
    {57, 46},
    {58, 47},
    {59, 48},
    {60, 49},
    {61, 50},
    {62, 51},
    {63, 52},
    {64, 53},
    {65, 54},
    {66, 55},
    {67, 56},
    {68, 57},
    {100, 58},
    {101, 59},
    {102, 60},
    {103, 61},
    {104, 62},
    {105, 63},
    {106, 64},
    {108, 65},
    {110, 66},
    {111, 67},
    {113, 68},
    {114, 69},
    {115, 70},
    {116, 71},
    {119, 72},
    {120, 73},
};
#define ZEN_NAUDIO ((int)(sizeof k_zen_audio / sizeof k_zen_audio[0]))

#endif
