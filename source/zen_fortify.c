/* zen_fortify.c -- the bionic FORTIFY checks libgameDSO.so imports and the
 * runtime does not have yet (__strlen_chk and its family; labs is passed
 * through to newlib by tools/imports.cfg).
 *
 * Each does what bionic's does -- the plain call, after checking the size the
 * compiler passed -- and stops the game with the caller's address in the log
 * when a buffer would overflow, as bionic aborted. MIT.
 */
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "error.h"

#define CHK_FAIL(what, a, b)                                                                        \
  fatal_error("%s: %u bytes into a buffer of %u (from %p)", what, (unsigned)(a), (unsigned)(b), \
              __builtin_return_address(0))

size_t b___strlen_chk(const char *s, size_t s_len) {
  size_t n = strlen(s);
  if (n >= s_len)
    CHK_FAIL("__strlen_chk", n + 1, s_len);
  return n;
}

char *b___strchr_chk(const char *s, int ch, size_t s_len) {
  const char *e = memchr(s, 0, s_len);
  if (!e && s_len != (size_t)-1)
    CHK_FAIL("__strchr_chk", s_len + 1, s_len);
  return strchr(s, ch);
}

char *b___strcpy_chk(char *dst, const char *src, size_t dst_len) {
  size_t n = strlen(src);
  if (n >= dst_len)
    CHK_FAIL("__strcpy_chk", n + 1, dst_len);
  return strcpy(dst, src);
}

char *b___strcat_chk(char *dst, const char *src, size_t dst_len) {
  size_t need = strlen(dst) + strlen(src) + 1;
  if (need > dst_len)
    CHK_FAIL("__strcat_chk", need, dst_len);
  return strcat(dst, src);
}

char *b___strncat_chk(char *dst, const char *src, size_t n, size_t dst_len) {
  size_t add = strnlen(src, n);
  size_t need = strlen(dst) + add + 1;
  if (need > dst_len)
    CHK_FAIL("__strncat_chk", need, dst_len);
  return strncat(dst, src, n);
}

int b___vsprintf_chk(char *dst, int flags, size_t dst_len, const char *fmt, va_list ap) {
  (void)flags;
  /* bionic passes (size_t)-1 when the compiler could not tell the buffer's size:
   * nothing to check against, and newlib's vsnprintf refuses sizes over INT_MAX
   * (it returns -1), so that case is the plain vsprintf. */
  if (dst_len > 0x7fffffffu)
    return vsprintf(dst, fmt, ap);
  int n = vsnprintf(dst, dst_len, fmt, ap);
  if (n < 0 || (size_t)n >= dst_len)
    CHK_FAIL("__vsprintf_chk", n + 1, dst_len);
  return n;
}

/* ZENONIA 2 imports these two (ZENONIA 3 only the second) */
char *b___strncpy_chk(char *dst, const char *src, size_t n, size_t dst_len) {
  if (n > dst_len)
    CHK_FAIL("__strncpy_chk", n, dst_len);
  return strncpy(dst, src, n);
}

/* bionic's pthread_atfork hook: this process never forks */
int b___register_atfork(void (*prepare)(void), void (*parent)(void), void (*child)(void), void *dso) {
  (void)prepare;
  (void)parent;
  (void)child;
  (void)dso;
  return 0;
}
