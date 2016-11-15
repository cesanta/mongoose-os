/*
* Copyright (c) 2015 Cesanta Software Limited
* All rights reserved
*/

#include <ctype.h>
#include <sys/time.h>
#include <stdint.h>
#include "common/platforms/esp8266/esp_missing_includes.h"
#include "common/cs_strtod.h"

#include <math.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <stddef.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include <ets_sys.h>
#include <os_type.h>
#include <osapi.h>
#include <mem.h>
#include <user_interface.h>
#include <errno.h>

#include "fw/platforms/esp8266/user/esp_features.h"

#if MIOT_ENABLE_HEAP_LOG
int cs_heap_shim = 0;
#define CS_HEAP_SHIM_FLAG_SET() \
  do {                          \
    cs_heap_shim = 1;           \
  } while (0)
#else
#define CS_HEAP_SHIM_FLAG_SET()
#endif

/* #define ESP_ABORT_ON_MALLOC_FAILURE */

/*
 * strerror provided by libc consumes 2kb RAM
 * Moreover, V7 uses strerror mostly for
 * file operation, so returns of strerror
 * are undefined, because spiffs uses its own
 * error codes and doesn't provide
 * error descriptions
 */
char *strerror(int errnum) {
  static char buf[15];
  snprintf(buf, sizeof(buf), "err: %d", errnum);
  buf[sizeof(buf) - 1] = 0;
  return buf;
}

void *malloc(size_t size) {
  void *res;
  CS_HEAP_SHIM_FLAG_SET();
  res = (void *) os_malloc(size);
#ifdef ESP_ABORT_ON_MALLOC_FAILURE
  if (res == NULL) abort();
#endif
  return res;
}

void free(void *ptr) {
  CS_HEAP_SHIM_FLAG_SET();
  os_free(ptr);
}

void *realloc(void *ptr, size_t size) {
  void *res;
  CS_HEAP_SHIM_FLAG_SET();
  /* ESP realloc is annoying - it prints an error message if reallocing to 0. */
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  res = (void *) os_realloc(ptr, size);
#ifdef ESP_ABORT_ON_MALLOC_FAILURE
  if (res == NULL) {
    printf("failed to alloc %u bytes, %d avail\n", size,
           system_get_free_heap_size());
    abort();
  }
#endif
  return res;
}

void *calloc(size_t num, size_t size) {
  void *res;
  CS_HEAP_SHIM_FLAG_SET();
  res = (void *) os_zalloc(num * size);
#ifdef ESP_ABORT_ON_MALLOC_FAILURE
  if (res == NULL) abort();
#endif
  return res;
}

#ifndef LWIP_OPEN_SRC

uint32_t htonl(uint32_t hostlong) {
  return ((hostlong & 0xff000000) >> 24) | ((hostlong & 0x00ff0000) >> 8) |
         ((hostlong & 0x0000ff00) << 8) | ((hostlong & 0x000000ff) << 24);
}

uint16_t htons(uint16_t hostshort) {
  return ((hostshort & 0xff00) >> 8) | ((hostshort & 0x00ff) << 8);
}

uint16_t ntohs(uint16_t netshort) {
  return htons(netshort);
}

uint32_t ntohl(uint32_t netlong) {
  return htonl(netlong);
}

#endif

void *_malloc_r(struct _reent *r, size_t size) {
  (void) r;
  return malloc(size);
}

void *_calloc_r(struct _reent *r, size_t nmemb, size_t size) {
  (void) r;
  return calloc(nmemb, size);
}

void _free_r(struct _reent *r, void *ptr) {
  (void) r;
  free(ptr);
}

void *_realloc_r(struct _reent *r, void *ptr, size_t size) {
  (void) r;
  return realloc(ptr, size);
}

NOINSTR void abort(void) {
  /* cause an unaligned access exception, that will drop you into gdb */
  *(int *) 1 = 1;
  while (1)
    ; /* avoid gcc warning because stdlib abort() has noreturn attribute */
}

void _exit(int status) {
  printf("_exit(%d)\n", status);
  abort();
}

/*
 * This will prevent counter wrap if time is read regularly.
 * At least Mongoose poll queries time, so we're covered.
 */
int _gettimeofday_r(struct _reent *r, struct timeval *tp, void *tzp) {
  static uint32_t prev_time = 0;
  static uint32_t num_overflows = 0;
  uint32_t time = system_get_time();
  uint64_t time64 = time;
  if (prev_time > 0 && time < prev_time) num_overflows++;
  time64 += (((uint64_t) num_overflows) * (1ULL << 32));
  tp->tv_sec = time64 / 1000000ULL;
  tp->tv_usec = time64 % 1000000ULL;
  prev_time = time;
  return 0;
  (void) r;
  (void) tzp;
}
