#include "test_util.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif

int num_tests = 0;

static char *_escape(const char *s, size_t n) {
  size_t i, j;
  char *res = (char *) malloc(n * 4 + 1);
  for (i = j = 0; s[i] != '\0' && i < n; i++) {
    if (!iscntrl((int) s[i])) {
      res[j++] = s[i];
    } else {
      j += sprintf(res + j, "\\x%02x", s[i]);
    }
  }
  res[j] = '\0';
  return res;
}

void _strfail(const char *a, const char *e, int len) {
  char *ae, *ee;
  if (len < 0) {
    len = strlen(a);
    if (strlen(e) > (size_t) len) len = strlen(e);
  }
  ae = _escape(a, len);
  ee = _escape(e, len);
  printf("Expected: %s\nActual  : %s\n", ee, ae);
  free(ae);
  free(ee);
}

double _now() {
  double now;
#ifndef _WIN32
  struct timeval tv;
  if (gettimeofday(&tv, NULL /* tz */) != 0) return 0;
  now = (double) tv.tv_sec + (((double) tv.tv_usec) / 1000000.0);
#else
  now = GetTickCount() / 1000.0;
#endif
  return now;
}

int _assert_streq(const char *actual, const char *expected) {
  if (strcmp(actual, expected) != 0) {
    _strfail(actual, expected, -1);
    return 0;
  }
  return 1;
}

int _assert_streq_nz(const char *actual, const char *expected) {
  size_t n = strlen(expected);
  if (strncmp(actual, expected, n) != 0) {
    _strfail(actual, expected, n);
    return 0;
  }
  return 1;
}
