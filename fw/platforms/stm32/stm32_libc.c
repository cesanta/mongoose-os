/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "FreeRTOS.h"

#include "mgos_debug.h"

#include "stm32_uart.h"

#include <stm32_sdk_hal.h>

int _gettimeofday(struct timeval *tv, void *tzvp) {
  uint32_t tick_ms = HAL_GetTick();
  tv->tv_sec = tick_ms / 1000;
  tv->tv_usec = (tv->tv_sec - tick_ms / 1000) / 1000;
  return 0;
}

int settimeofday(const struct timeval *tv, const struct timezone *tz) {
  /* TODO(rojer) */
  return -1;
}

void abort(void) {
  fflush(stdout);
  fflush(stderr);
  mgos_debug_flush();
  void *sp;
  __asm volatile("mov %0, sp" : "=r"(sp) : :);
  stm32_uart_dprintf("\nabort() called, sp = %p\n", sp);
  __builtin_trap();  // Executes an illegal instruction.
}

void __malloc_lock(void) {
  portENTER_CRITICAL();
}

void __malloc_unlock(void) {
  portEXIT_CRITICAL();
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
