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

#ifndef MGOS_ENABLE_CALL_TRACE
#define MGOS_ENABLE_CALL_TRACE 0
#endif

#ifndef V7_ENABLE_CALL_TRACE
#define V7_ENABLE_CALL_TRACE 0
#endif

#if MGOS_ENABLE_CALL_TRACE || V7_ENABLE_CALL_TRACE
/*
 * If we don't have V7's profiling functions, roll our own.
 * This is copy-pasta from v7/src/cyg_profile.c
 */

#ifndef CALL_TRACE_SIZE
#define CALL_TRACE_SIZE 32
#endif

typedef struct {
  void *addresses[CALL_TRACE_SIZE];
  uint16_t size;
} call_trace_t;

static call_trace_t call_trace;

#if MGOS_ENABLE_CALL_TRACE
void esp_exc_printf(const char *fmt, ...);
#define call_trace_printf esp_exc_printf
#else
#define call_trace_printf printf
#endif

NOINSTR void print_call_trace() {
  static void *prev_trace[CALL_TRACE_SIZE];
  unsigned int size = call_trace.size;
  if (size > CALL_TRACE_SIZE) size = CALL_TRACE_SIZE;
  unsigned int i;
  uintptr_t pa = 0;
  for (i = 0; i < size; i++) {
    if (call_trace.addresses[i] != prev_trace[i]) break;
    pa = (uintptr_t) call_trace.addresses[i];
  }
  call_trace_printf("%u %u", size, i);
  for (; i < size; i++) {
    const uintptr_t a = (uintptr_t) call_trace.addresses[i];
    /*
     * Perform a rudimentary deduplication: an address is likely to have higher
     * bits the same as previous, turn them off.
     * Do it in 4-bit nibbles so they fall nicely on hex digit boundary.
     */
    uintptr_t mask = ~((uintptr_t) 0);
    while (mask != 0 && (a & mask) != (pa & mask)) mask <<= 4;
    call_trace_printf(" %lx", (unsigned long) (a & ~mask));
    prev_trace[i] = (void *) a;
    pa = a;
  }
  call_trace_printf("\n");
}

#if MGOS_ENABLE_CALL_TRACE && !V7_ENABLE_CALL_TRACE
IRAM NOINSTR void __cyg_profile_func_enter(void *this_fn, void *call_site) {
  if (call_trace.size < CALL_TRACE_SIZE) {
    call_trace.addresses[call_trace.size] = this_fn;
  }
  call_trace.size++;
  (void) this_fn;
  (void) call_site;
}

IRAM NOINSTR void __cyg_profile_func_exit(void *this_fn, void *call_site) {
  if (call_trace.size > 0) call_trace.size--;
  (void) this_fn;
  (void) call_site;
}
#endif

#endif /* MGOS_ENABLE_CALL_TRACE || V7_ENABLE_CALL_TRACE */
