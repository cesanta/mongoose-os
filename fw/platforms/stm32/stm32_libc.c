#include <stdio.h>
#include <stm32_sdk_hal.h>

int _gettimeofday(struct timeval *tv, void *tzvp) {
  uint32_t tick_ms = HAL_GetTick();
  tv->tv_sec = tick_ms / 1000;
  tv->tv_usec = (tv->tv_sec - tick_ms / 1000) / 1000;
  return 0;
}
