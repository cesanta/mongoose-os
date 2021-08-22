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

#include "esp_exc.h"
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
  esp_check_stack_overflow(1, (int) size, NULL);
  res = (void *) umm_malloc(size);
  esp_check_stack_overflow(1, (int) size, res);
#ifdef ESP_ABORT_ON_MALLOC_FAILURE
  if (res == NULL && size != 0) abort();
#endif
  return res;
}

void free(void *ptr) {
  CS_HEAP_SHIM_FLAG_SET();
  esp_check_stack_overflow(2, 0, ptr);
  umm_free(ptr);
  esp_check_stack_overflow(2, 1, ptr);
}

void *realloc(void *ptr, size_t size) {
  void *res;
  CS_HEAP_SHIM_FLAG_SET();
  esp_check_stack_overflow(3, (int) size, ptr);
  res = (void *) umm_realloc(ptr, size);
#ifdef ESP_ABORT_ON_MALLOC_FAILURE
  if (res == NULL && size != 0) abort();
#endif
  esp_check_stack_overflow(4, (int) size, res);
  return res;
}

void *calloc(size_t num, size_t size) {
  void *res;
  CS_HEAP_SHIM_FLAG_SET();
  esp_check_stack_overflow(5, (int) (num * size), NULL);
  res = (void *) umm_calloc(num, size);
#ifdef ESP_ABORT_ON_MALLOC_FAILURE
  if (res == NULL && size != 0) abort();
#endif
  esp_check_stack_overflow(5, (int) (num * size), res);
  return res;
}

// Allocation functions used by the SDK.
// Note that SDK function prototypes have additional arguments that we ignore.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-attributes"
// void *pvPortMalloc(size_t size, const char *file, unsigned line, unsigned
// iram) SDK 3.0 added "iram" parameter, presumably to allocate memory from
// IRAM. It's not documented and has only been seen to be used for minor DHCP
// stuff.
void *pvPortMalloc(size_t size) __attribute__((alias("malloc")));

// void *pvPortRealloc(void *ptr, size_t size, const char *file, unsigned line)
void *pvPortRealloc(void *ptr, size_t size) __attribute__((alias("realloc")));

// void vPortFree(void *ptr, const char *file, unsigned line) {
void vPortFree(void *ptr) __attribute__((alias("free")));
#pragma GCC diagnostic pop

void *pvPortZalloc(size_t size, const char *file, unsigned line) {
  (void) file;
  (void) line;
  return calloc(1, size);
}

size_t xPortGetFreeHeapSize(void) {
  return umm_free_heap_size();
}

size_t xPortWantedSizeAlign(void) {
  return 8;
}

void esp_umm_init(void) {
  /* Nothing to do, see header for details */
}

void esp_umm_oom_cb(size_t size, size_t blocks_cnt) {
  fprintf(stderr, "E:M %u (%u blocks)\n", (unsigned int) size,
          (unsigned int) blocks_cnt);
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

IRAM int64_t mgos_uptime_micros(void) {
  static volatile uint32_t t[2] = {0, 0};
  uint32_t time = system_get_time();
  uint32_t prev_time = t[0], num_overflows = t[1];
  /*
   * Test for wraparound. This may get preempted and race (e.g. with ISR).
   * We don't want to disable ints for every call to this function,
   * hence the trickery.
   */
  if (((time ^ prev_time) & (1UL << 31)) != 0 && (time & (1UL << 31)) == 0) {
    mgos_ints_disable();
    if (t[1] == num_overflows) {
      /* We got here first, update the global overflow counter. */
      t[1] = ++num_overflows;
      t[0] = time;
    } else {
      /* We raced and lost, only update the local copy of the counter. */
      num_overflows++;
    }
    mgos_ints_enable();
  } else {
    t[0] = time;
  }
  int64_t time64 = time;
  time64 += (((uint64_t) num_overflows) * (1ULL << 32));
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
