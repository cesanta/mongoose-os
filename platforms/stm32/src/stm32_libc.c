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

#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "FreeRTOS.h"
#include "task.h"

#include "mgos_core_dump.h"
#include "mgos_debug.h"
#include "mgos_system.h"
#include "mgos_time.h"

#include "stm32_system.h"
#include "stm32_uart_internal.h"

#include "umm_malloc.h"

#include "stm32_sdk_hal.h"

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

void abort(void) {
#if CS_ENABLE_STDIO
  fflush(stdout);
  fflush(stderr);
  mgos_debug_flush();
  void *sp;
  __asm volatile("mov %0, sp" : "=r"(sp) : :);
  mgos_cd_printf("\nabort() called, sp = %p\n", sp);
#endif
  __builtin_trap();  // Executes an illegal instruction.
}

void *malloc(size_t size) {
  return umm_malloc(size);
}

void *_malloc_r(struct _reent *r, size_t size) {
  (void) r;
  return umm_malloc(size);
}

void free(void *ptr) {
  umm_free(ptr);
}

void _free_r(struct _reent *r, void *ptr) {
  (void) r;
  return umm_free(ptr);
}

void *calloc(size_t nmemb, size_t size) {
  return umm_calloc(nmemb, size);
}

void *_calloc_r(struct _reent *r, size_t nmemb, size_t size) {
  (void) r;
  return umm_calloc(nmemb, size);
}

void *realloc(void *ptr, size_t size) {
  return umm_realloc(ptr, size);
}

void *_realloc_r(struct _reent *r, void *ptr, size_t size) {
  (void) r;
  return umm_realloc(ptr, size);
}

size_t mgos_get_heap_size(void) {
  return &_heap_end - &_heap_start;
}

size_t mgos_get_free_heap_size(void) {
  return umm_free_heap_size();
}

size_t mgos_get_min_free_heap_size(void) {
  return umm_min_free_heap_size();
}

void umm_oom_cb(size_t size, size_t blocks_cnt) {
  fprintf(stderr, "E:M %u (%u blocks)\n", (unsigned int) size,
          (unsigned int) blocks_cnt);
}
