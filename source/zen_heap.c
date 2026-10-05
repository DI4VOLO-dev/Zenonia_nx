/* zen_heap.c -- the game's malloc / free, and its log, as this port serves them.
 *
 * Why not the plain newlib calls (the first hardware runs, 2026-10-04):
 *
 *  - The engine's own allocator wrapper (MC_knlCalloc / MC_knlFree) is a thin
 *    layer over malloc/free, and its way of working relied on how Android's
 *    allocator behaves: it frees whole groups of blocks at a state change (a
 *    burst of ~6400 frees right before every crash in the logs) and reads
 *    objects that were just freed, or past the end of a block by a few bytes.
 *    Android's dlmalloc leaves a freed block's bytes alone and gives the new
 *    owner memory that is not reused at once; newlib's recycles and overwrites
 *    them at once. Three crashes of four were a state manager whose first word
 *    had become 0x10, the fourth a string table that had become NULL: both
 *    "an object read after it was freed".
 *  - So: zen_malloc hands out zeroed blocks with 32 bytes of slack behind them
 *    and a 16-byte header (which keeps the 8-byte alignment); zen_free puts the
 *    block in a QUARANTINE (the last ZEN_QUAR_BYTES of freed memory, untouched,
 *    not reusable) and only then really frees the oldest. A double free, or a
 *    free of a pointer that did not come from here, is logged and ignored.
 *
 *  - Every MC_knlCalloc / MC_knlFree wrote a line through __android_log_print
 *    (6000+ lines a load, with an SD-card sync every 16 lines while booting:
 *    a single frame took 23 s, and the first start 10 s). Those lines say
 *    nothing but a counter; they are dropped here, every other log line goes
 *    on to the runtime's log as before. MIT.
 */
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <switch.h>

#include "imports.h"
#include "util.h"

#define ZEN_HDR 16u
#define ZEN_SLACK 32u
#define ZEN_ALIVE 0x7a656e41u /* "zenA" */
#define ZEN_FREED 0x7a656e46u /* "zenF" */
#define ZEN_QUAR_BYTES (48u * 1024 * 1024)
#define ZEN_QUAR_MAX 65536u

typedef struct {
  uint32_t magic;
  uint32_t size;
  uint32_t pad[2];
} Hdr;

static Mutex g_lock;
static int g_init;
static void *g_ring[ZEN_QUAR_MAX];
static uint32_t g_head, g_count;
static size_t g_quar_bytes;
static unsigned g_bad_logged;
static unsigned long g_allocs, g_frees, g_bad;

static void init_once(void) {
  if (!g_init) {
    mutexInit(&g_lock);
    g_init = 1;
  }
}

void *zen_malloc(size_t n) {
  init_once();
  if (n > 0x7fffffffu - ZEN_HDR - ZEN_SLACK)
    return NULL;
  size_t total = ZEN_HDR + n + ZEN_SLACK;
  uint8_t *p = malloc(total);
  if (!p) {
    /* out of memory: give the quarantine back and try once more */
    mutexLock(&g_lock);
    while (g_count) {
      uint32_t tail = (g_head + ZEN_QUAR_MAX - g_count) % ZEN_QUAR_MAX;
      free(g_ring[tail]);
      g_count--;
    }
    g_quar_bytes = 0;
    mutexUnlock(&g_lock);
    p = malloc(total);
    if (!p)
      return NULL;
  }
  memset(p, 0, total);
  Hdr *h = (Hdr *)p;
  h->magic = ZEN_ALIVE;
  h->size = (uint32_t)n;
  g_allocs++;
  return p + ZEN_HDR;
}

void zen_free(void *q) {
  if (!q)
    return;
  init_once();
  Hdr *h = (Hdr *)((uint8_t *)q - ZEN_HDR);
  if (h->magic != ZEN_ALIVE) {
    g_bad++;
    if (g_bad_logged < 20) {
      g_bad_logged++;
      debugPrintf("[heap] free(%p): %s (from %p) -- ignored\n", q,
                  h->magic == ZEN_FREED ? "double free" : "not a block of this heap",
                  __builtin_return_address(0));
    }
    return;
  }
  h->magic = ZEN_FREED;
  g_frees++;
  mutexLock(&g_lock);
  g_ring[g_head] = h;
  g_head = (g_head + 1) % ZEN_QUAR_MAX;
  if (g_count < ZEN_QUAR_MAX)
    g_count++;
  g_quar_bytes += (size_t)h->size + ZEN_HDR + ZEN_SLACK;
  /* really free the oldest while over the limit */
  while (g_count && (g_quar_bytes > ZEN_QUAR_BYTES || g_count == ZEN_QUAR_MAX)) {
    uint32_t tail = (g_head + ZEN_QUAR_MAX - g_count) % ZEN_QUAR_MAX;
    Hdr *old = g_ring[tail];
    g_quar_bytes -= (size_t)old->size + ZEN_HDR + ZEN_SLACK;
    g_count--;
    old->magic = 0;
    free(old);
  }
  mutexUnlock(&g_lock);
}

/* [heap] line for the periodic report (zen_main.c) */
void zen_heap_report(void) {
  debugPrintf("[heap] game blocks: %lu allocated, %lu freed, %lu in quarantine (%u KB), %lu bad frees\n", g_allocs, g_frees,
              (unsigned long)g_count, (unsigned)(g_quar_bytes >> 10), g_bad);
}

/* ------------------------------------------------------------------ log */
extern int b___android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap);

int zen_android_log_print(int prio, const char *tag, const char *fmt, ...) {
  if (fmt && strstr(fmt, "MCknl"))
    return 0; /* MC_knlCalloc / MC_knlFree counters: thousands of lines a load */
  va_list ap;
  va_start(ap, fmt);
  int r = b___android_log_vprint(prio, tag, fmt, ap);
  va_end(ap);
  return r;
}

/* ------------------------------------------------------------------ the overlay: looked up before dcr_imports */
const DynLibFunction port_imports[] = {
    {"malloc", (uintptr_t)zen_malloc},
    {"free", (uintptr_t)zen_free},
    {"__android_log_print", (uintptr_t)zen_android_log_print},
};
const int port_imports_count = 3;
