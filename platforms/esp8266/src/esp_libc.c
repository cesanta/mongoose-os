/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include "esp_missing_includes.h"
#include "umm_malloc.h"

#ifdef RTOS_SDK
#include "esp_system.h"
#else
#include "user_interface.h"
#endif

#include "esp_features.h"
#include "mgos_hal.h"
#include "mgos_time.h"

#if MGOS_ENABLE_HEAP_LOG
int cs_heap_shim = 0;
#define CS_HEAP_SHIM_FLAG_SET() \
  do {                          \
    cs_heap_shim = 1;           \
  } while (0)
#else
#define CS_HEAP_SHIM_FLAG_SET()
#endif

/* #define ESP_ABORT_ON_MALLOC_FAILURE */

void *malloc(size_t size) {
  void *res;
  CS_HEAP_SHIM_FLAG_SET();
  res = (void *) umm_malloc(size);
#ifdef ESP_ABORT_ON_MALLOC_FAILURE
  if (res == NULL) abort();
#endif
  return res;
}

void free(void *ptr) {
  CS_HEAP_SHIM_FLAG_SET();
  umm_free(ptr);
}

void *realloc(void *ptr, size_t size) {
  void *res;
  CS_HEAP_SHIM_FLAG_SET();
  res = (void *) umm_realloc(ptr, size);
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
  res = (void *) umm_calloc(num, size);
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

void _exit(int status) {
  printf("_exit(%d)\n", status);
  abort();
}

/*
 * This will prevent counter wrap if time is read regularly.
 * At least Mongoose poll queries time, so we're covered.
 */

IRAM int64_t mgos_uptime_micros(void) {
  static uint32_t prev_time = 0;
  static uint32_t num_overflows = 0;
  uint32_t time = system_get_time();
  int64_t time64 = time;
  if (prev_time > 0 && time < prev_time) num_overflows++;
  time64 += (((uint64_t) num_overflows) * (1ULL << 32));
  prev_time = time;
  return time64;
}

static int64_t sys_time_adj = 0;

int _gettimeofday_r(struct _reent *r, struct timeval *tv, struct timezone *tz) {
  int64_t time64 = mgos_uptime_micros() + sys_time_adj;
  tv->tv_sec = time64 / 1000000ULL;
  tv->tv_usec = time64 % 1000000ULL;
  return 0;
  (void) r;
  (void) tz;
}

int settimeofday(const struct timeval *tv, const struct timezone *tz) {
  int64_t time64_cur = mgos_uptime_micros();
  int64_t time64_new = tv->tv_sec * 1000000LL + tv->tv_usec;
  sys_time_adj = (time64_new - time64_cur);
  (void) tz;
  return 0;
}
