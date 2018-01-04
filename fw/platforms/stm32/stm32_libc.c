/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <sys/time.h>
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
