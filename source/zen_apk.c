/* zen_apk.c -- the game's files, read from the player's APK.
 *
 * ZENONIA's Java side read everything through the AssetManager (assets/) and
 * Resources (res/raw/: the sounds); here both come from the APK itself, so no
 * game file is ever copied or shipped. The APK is opened once with miniz and
 * read from the game's own thread (the sounds are read up front by
 * zen_audio_init, so the audio thread never touches it). MIT.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "miniz/miniz.h"
#include "util.h"
#include "zen.h"

static mz_zip_archive g_zip;
static int g_open;

int zen_apk_init(const char *apk) {
  if (g_open)
    return 0;
  memset(&g_zip, 0, sizeof g_zip);
  if (!mz_zip_reader_init_file(&g_zip, apk, 0)) {
    debugPrintf("[apk] cannot open %s as a zip\n", apk);
    return -1;
  }
  g_open = 1;
  debugPrintf("[apk] %s: %u entries\n", apk, (unsigned)mz_zip_reader_get_num_files(&g_zip));
  return 0;
}

uint8_t *zen_apk_read(const char *entry, size_t *len) {
  if (!g_open || !entry)
    return NULL;
  int idx = mz_zip_reader_locate_file(&g_zip, entry, NULL, 0);
  if (idx < 0)
    return NULL;
  size_t n = 0;
  void *p = mz_zip_reader_extract_to_heap(&g_zip, (mz_uint)idx, &n, 0);
  if (!p)
    return NULL;
  if (len)
    *len = n;
  return (uint8_t *)p;
}

long zen_apk_size(const char *entry) {
  if (!g_open || !entry)
    return -1;
  int idx = mz_zip_reader_locate_file(&g_zip, entry, NULL, 0);
  mz_zip_archive_file_stat st;
  if (idx < 0 || !mz_zip_reader_file_stat(&g_zip, (mz_uint)idx, &st))
    return -1;
  return (long)st.m_uncomp_size;
}
