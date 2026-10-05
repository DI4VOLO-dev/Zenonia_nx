/* zen_java.c -- the game's Java, as the engine sees it.
 *
 * libgameDSO.so never looks an object up: everything it calls on the Java side
 * is a static method of one class, com.gamevil.nexus2.Natives (the names are
 * in the library; the bodies were in classes.dex):
 *
 *   readAssete, isAssetExist        the APK's assets/                zen_apk.c
 *   loadFile, isFileExist           the app's files folder
 *   getAbsolueFilePath              that folder (/data/data/<pkg>/files)
 *   OnSoundPlay, OnStopSound,       MediaPlayer, on the mixer        zen_audio.c
 *     playAndroidSound, stopAndroidSound
 *   SetSpeed                        the frame rate (GLThread.SetFPS(n + 2))
 *   getGLOptionLinear               the texture filter option
 *   finishApp                       the game asks to close
 *   OnVibrate, OnUIStatusChange, changeUIStatus, OnMessage
 *                                   the phone's vibrator and its Views: nothing
 *   getPhoneModel, getPhoneNumber, getPlayerNameByte, getPlayerNumberByte
 *                                   identity strings, as bytes
 *
 * and a lot of names the 1.6.6 library still carries from older builds of the
 * engine (the store, Tapjoy, the server sockets, dialogs: requestIAP,
 * netConnect, showLoadingDialog...) that dex no longer has. Every one of them
 * answers "nothing / not available" by the type it returns (h_default), which
 * is what a phone without them would have said. The library's own checks
 * (isNetAvailable) say there is no network. MIT.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <switch.h>

#include "dcr_path.h"
#include "jni.h"
#include "rt_applet.h"
#include "rt_cfg.h"
#include "util.h"
#include "zen.h"
#include "zen_config.h"

#define NAT ZEN_NATIVES

static int g_fps = 25; /* NexusGLThread's constructor: SetFPS(25) */
int zen_fps(void) { return g_fps; }
int zen_smooth(void) { return zen_config()->smooth; }

/* ------------------------------------------------------------------ helpers */
static JObj *bytes(const void *p, size_t n) {
  JObj *o = jni_array('B', (jsize)n);
  if (o && n)
    memcpy(o->a.data, p, n);
  return o;
}

/* "x/y" -> "assets/x/y": AssetManager.open's names have no leading slash */
static void asset_entry(const char *name, char *out, size_t cap) {
  while (*name == '/')
    name++;
  snprintf(out, cap, "assets/%s", name);
}

/* Context.openFileInput(name): a file in the app's files folder */
static int local_path(const char *name, char *out, size_t cap) {
  char android[DCR_PATH_MAX];
  while (*name == '/')
    name++;
  snprintf(android, sizeof android, "%s/%s", DCR_ANDROID_FILES, name);
  const char *real = dcr_translate_path(android, out, cap);
  if (real != out)
    snprintf(out, cap, "%s", real);
  return 0;
}

/* ------------------------------------------------------------------ files */
static jvalue h_read_assete(JObj *self, const jvalue *a, const JMethod *m) {
  static int logged;
  char entry[DCR_PATH_MAX];
  size_t n = 0;
  asset_entry(jni_utf(a[0].l), entry, sizeof entry);
  uint8_t *p = zen_apk_read(entry, &n);
  if (!p) {
    debugPrintf("[java] readAssete(%s): NOT in the APK\n", entry);
    return jv_l(NULL); /* the Java caught its FileNotFoundException and gave null */
  }
  if (logged < 400) { /* the first 400 reads, to see what the game loads (debug.log) */
    logged++;
    debugPrintf("[java] readAssete(%s): %u bytes\n", entry, (unsigned)n);
  }
  JObj *o = bytes(p, n);
  free(p);
  return jv_l(o);
}

static jvalue h_is_asset_exist(JObj *self, const jvalue *a, const JMethod *m) {
  char entry[DCR_PATH_MAX];
  asset_entry(jni_utf(a[0].l), entry, sizeof entry);
  long n = zen_apk_size(entry);
  return jv_i(n < 0 ? 0 : (jint)n); /* InputStream.available(): the size; 0 for none */
}

static jvalue h_is_file_exist(JObj *self, const jvalue *a, const JMethod *m) {
  char path[DCR_PATH_MAX];
  struct stat st;
  local_path(jni_utf(a[0].l), path, sizeof path);
  int ok = stat(path, &st) == 0;
  debugPrintf("[java] isFileExist(%s): %s\n", path, ok ? "yes" : "no");
  return jv_i(ok ? (jint)st.st_size : 0);
}

static jvalue h_load_file(JObj *self, const jvalue *a, const JMethod *m) {
  char path[DCR_PATH_MAX];
  local_path(jni_utf(a[0].l), path, sizeof path);
  FILE *f = fopen(path, "rb");
  if (!f) {
    debugPrintf("[java] loadFile(%s): cannot open\n", path);
    return jv_l(NULL);
  }
  fseek(f, 0, SEEK_END);
  long n = ftell(f);
  fseek(f, 0, SEEK_SET);
  JObj *o = NULL;
  if (n >= 0) {
    o = jni_array('B', (jsize)n);
    if (o && n && fread(o->a.data, 1, (size_t)n, f) != (size_t)n)
      debugPrintf("[java] loadFile(%s): short read\n", path);
  }
  fclose(f);
  return jv_l(o);
}

static jvalue h_files_dir(JObj *self, const jvalue *a, const JMethod *m) { return jv_l(jni_str(DCR_ANDROID_FILES)); }

/* ------------------------------------------------------------------ sound */
static jvalue h_sound_play(JObj *self, const jvalue *a, const JMethod *m) {
  zen_audio_play(a[0].i, a[1].i, a[2].z);
  return jv_none();
}
static jvalue h_sound_stop(JObj *self, const jvalue *a, const JMethod *m) {
  zen_audio_stop_all();
  return jv_none();
}

/* ------------------------------------------------------------------ the rest */
static jvalue h_set_speed(JObj *self, const jvalue *a, const JMethod *m) {
  int fps = a[0].i + 2; /* Natives.SetSpeed: NexusGLThread.SetFPS(n + 2) */
  if (fps < 5)
    fps = 5;
  if (fps > 60)
    fps = 60;
  if (fps != g_fps)
    debugPrintf("[java] SetSpeed(%d): %d fps\n", (int)a[0].i, fps);
  g_fps = fps;
  return jv_none();
}
static jvalue h_finish(JObj *self, const jvalue *a, const JMethod *m) {
  debugPrintf("[java] finishApp: the game asked to close\n");
  rt_request_exit();
  return jv_none();
}
static jvalue h_smooth(JObj *self, const jvalue *a, const JMethod *m) { return jv_i(zen_smooth()); }
static jvalue h_no_net(JObj *self, const jvalue *a, const JMethod *m) { return jv_i(0); }
static jvalue h_model(JObj *self, const jvalue *a, const JMethod *m) { return jv_l(bytes("Switch", 6)); }
static jvalue h_phone_number(JObj *self, const jvalue *a, const JMethod *m) {
  return jv_l(bytes("123456789012", 12)); /* what NexusGLActivity.getPhoneNumber gave */
}

/* nothing / not available, by the type the method returns: 0 / false, "", an
 * empty array, null */
static jvalue h_default(JObj *self, const jvalue *a, const JMethod *m) {
  switch (m->ret) {
  case 'V':
    return jv_none();
  case 'L':
    return strstr(m->sig, ")Ljava/lang/String;") ? jv_l(jni_str("")) : jv_l(NULL);
  case '[': {
    const char *p = strrchr(m->sig, ')');
    return jv_l(jni_array(p && p[1] == '[' && p[2] ? p[2] : 'B', 0));
  }
  case 'J':
    return jv_j(0);
  case 'F':
    return jv_f(0);
  case 'D':
    return jv_d(0);
  default:
    return jv_i(0);
  }
}

#define D(name) {NAT, name, NULL, h_default}

const JMethodDef jni_method_defs[] = {
    {NAT, "readAssete", NULL, h_read_assete},
    {NAT, "isAssetExist", NULL, h_is_asset_exist},
    {NAT, "isFileExist", NULL, h_is_file_exist},
    {NAT, "loadFile", NULL, h_load_file},
    {NAT, "getAbsolueFilePath", NULL, h_files_dir},
    {NAT, "OnSoundPlay", NULL, h_sound_play},
    {NAT, "playAndroidSound", NULL, h_sound_play},
    {NAT, "OnStopSound", NULL, h_sound_stop},
    {NAT, "stopAndroidSound", NULL, h_sound_stop},
    {NAT, "SetSpeed", NULL, h_set_speed},
    {NAT, "finishApp", NULL, h_finish},
    {NAT, "getGLOptionLinear", NULL, h_smooth},
    {NAT, "isNetAvailable", NULL, h_no_net},
    {NAT, "getPhoneModel", NULL, h_model},
    {NAT, "getPhoneNumber", NULL, h_phone_number},
    /* the phone's vibrator and Views */
    D("OnVibrate"), D("vibrateAndroid"), D("OnUIStatusChange"), D("changeUIStatus"), D("OnMessage"),
    D("OnEvent"), D("saveGLOptionLinear"), D("showLoadingDialog"), D("hideLoadingDialog"),
    D("showTitleComponent"), D("hideTitleComponent"), D("showReplyMoveComponent"), D("hideReplyMoveComponent"),
    D("blinkSButton"), D("showMylSButton"), D("NexusSetMaxLengthTextInput"), D("NetErrorDialog"),
    D("NetErrorDialogException"),
    /* identity the engine may ask for */
    D("getPlayerNameByte"), D("getPlayerNumberByte"), D("getVersion"), D("clearPlayerName"), D("getLanguage"),
    D("getLocaleID"), D("getGPoint"),
    /* the store, the ad network, the web, the server sockets and the tracker: not here */
    D("requestIAP"), D("startPurchase"), D("openPurchaseSuccess"), D("setPurchaseChange"),
    D("openStoreWithProductId"), D("openUrl"), D("setTapjoyReady"), D("trackPageViewDispatch"),
    D("trackEventDispatch"), D("getInetHostAddress"), D("netConnect"), D("netSocket"), D("netBillcomSocketConnect"),
    D("netBillcomSocketWrite"), D("netBillcomSocketRead"), D("netBillcomSocketClose"),
    {NULL, NULL, NULL, NULL},
};

const JFieldDef jni_field_defs[] = {
    {"android/os/Build$VERSION", "SDK_INT", NULL, 19, NULL}, /* the manifest's targetSdkVersion */
    {NULL, NULL, NULL, 0, NULL},
};
const char *const jni_class_supers[][2] = {{NULL, NULL}};
const char *const jni_missing_classes[] = {NAT, NULL};

void zen_java_init(void) {
  jni_init();
  g_jni_log = rt_config()->log_jni;
}
