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

#include "mgos.h"
#include "mgos_core_dump.h"
#include "mgos_debug.h"
#include "mgos_system.h"
#include "mgos_time.h"

#include "rs14100_sdk.h"

#include "FreeRTOS.h"
#include "task.h"

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

void __malloc_lock(struct _reent *r) {
  taskENTER_CRITICAL();
  (void) r;
}

void __malloc_unlock(struct _reent *r) {
  taskEXIT_CRITICAL();
  (void) r;
}

extern uint8_t _heap_start, _heap_end; /* Provided by linker */
void *_sbrk(intptr_t incr) {
  static uint8_t *cur_heap_end;
  uint8_t *ret, *new_heap_end;

  if (cur_heap_end == NULL) {
    memset(&_heap_start, 0, (&_heap_end - &_heap_start));
    cur_heap_end = &_heap_start;
  }

  new_heap_end = cur_heap_end + incr;
  if (new_heap_end <= &_heap_end) {
    ret = cur_heap_end;
    cur_heap_end = new_heap_end;
  } else {
    ret = (void *) -1;
  }

  return ret;
}

size_t mgos_get_heap_size(void) {
  return &_heap_end - &_heap_start;
}

size_t mgos_get_free_heap_size(void) {
  struct mallinfo mi = mallinfo();
  return mgos_get_heap_size() - mi.uordblks;
}

size_t mgos_get_min_free_heap_size(void) {
  return 0;
}
