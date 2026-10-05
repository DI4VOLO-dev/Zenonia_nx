/* zen_sounds.h -- the sound table of ZENONIA 1.6.6: the engine asks for a
 * sound by number (NexusSound.playSound: R.raw base + id, ids from 100 on
 * counted from 42), and the APK's resources.arsc names the file each raw
 * resource is. Generated from the APK's own table (the names are an
 * interface, not game content). MIT. */
#ifndef ZEN_SOUNDS_H
#define ZEN_SOUNDS_H
/* index: id for 0..41, id - 58 for 100..108 (the engine's own rule) */
static const char *const k_zen_wavs[] = {
    "res/bc9.wav", /* s000 */
    "res/bc8.wav", /* s001 */
    "res/bc7.wav", /* s002 */
    "res/bc6.wav", /* s003 */
    "res/bc5.wav", /* s004 */
    "res/bc4.wav", /* s005 */
    "res/bc3.wav", /* s006 */
    "res/bc2.wav", /* s007 */
    "res/bc1.wav", /* s008 */
    "res/bc.wav", /* s009 */
    "res/Lz9.wav", /* s010 */
    "res/Lz8.wav", /* s011 */
    "res/Lz7.wav", /* s012 */
    "res/Lz6.wav", /* s013 */
    "res/Lz5.wav", /* s014 */
    "res/Lz4.wav", /* s015 */
    "res/Lz3.wav", /* s016 */
    "res/Lz2.wav", /* s017 */
    "res/Lz1.wav", /* s018 */
    "res/Lz.wav", /* s019 */
    "res/7T9.wav", /* s020 */
    "res/7T8.wav", /* s021 */
    "res/7T7.wav", /* s022 */
    "res/7T6.wav", /* s023 */
    "res/7T5.wav", /* s024 */
    "res/7T4.wav", /* s025 */
    "res/7T3.wav", /* s026 */
    "res/7T2.wav", /* s027 */
    "res/7T1.wav", /* s028 */
    "res/7T.wav", /* s029 */
    "res/r-9.wav", /* s030 */
    "res/r-8.wav", /* s031 */
    "res/r-7.wav", /* s032 */
    "res/r-6.wav", /* s033 */
    "res/r-5.wav", /* s034 */
    "res/r-4.wav", /* s035 */
    "res/r-3.wav", /* s036 */
    "res/r-2.wav", /* s037 */
    "res/r-1.wav", /* s038 */
    "res/r-.wav", /* s039 */
    "res/bf1.wav", /* s040 */
    "res/bf.wav", /* s041 */
    "res/jh8.wav", /* s100 */
    "res/jh7.wav", /* s101 */
    "res/jh6.wav", /* s102 */
    "res/jh5.wav", /* s103 */
    "res/jh4.wav", /* s104 */
    "res/jh3.wav", /* s105 */
    "res/jh2.wav", /* s106 */
    "res/jh1.wav", /* s107 */
    "res/jh.wav", /* s108 */
};
#define ZEN_NWAVS ((int)(sizeof k_zen_wavs / sizeof k_zen_wavs[0]))
#endif
