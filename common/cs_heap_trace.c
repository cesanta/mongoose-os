/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>
#include <stdio.h>

#ifndef MIOT_ENABLE_CALL_TRACE
#define MIOT_ENABLE_CALL_TRACE 0
#endif

#ifndef V7_ENABLE_CALL_TRACE
#define V7_ENABLE_CALL_TRACE 0
#endif

#if MIOT_ENABLE_CALL_TRACE || V7_ENABLE_CALL_TRACE
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
  fprintf(stderr, "%u %u", size, i);
  for (; i < size; i++) {
    const uintptr_t a = (uintptr_t) call_trace.addresses[i];
    /*
     * Perform a rudimentary deduplication: an address is likely to have higher
     * bits the same as previous, turn them off.
     * Do it in 4-bit nibbles so they fall nicely on hex digit boundary.
     */
    uintptr_t mask = ~((uintptr_t) 0);
    while (mask != 0 && (a & mask) != (pa & mask)) mask <<= 4;
    fprintf(stderr, " %lx", (unsigned long) (a & ~mask));
    prev_trace[i] = (void *) a;
    pa = a;
  }
  fprintf(stderr, "\n");
}

#if MIOT_ENABLE_CALL_TRACE && !V7_ENABLE_CALL_TRACE
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

#endif /* MIOT_ENABLE_CALL_TRACE || V7_ENABLE_CALL_TRACE */
