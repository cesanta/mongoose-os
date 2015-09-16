#include "esp_missing_includes.h"
#include <v7.h>
#include <sj_timers.h>
#include <sj_v7_ext.h>
#include <sj_hal.h>

#include "v7_esp.h"
#include "sj_prompt.h"
#include "esp_uart.h"

#ifndef RTOS_SDK

#include <osapi.h>
#include <os_type.h>

#else

#include <eagle_soc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/timers.h>
#include "disp_task.h"

#endif /* RTOS_SDK */

size_t sj_get_free_heap_size() {
  return system_get_free_heap_size();
}

void sj_wdt_feed() {
  pp_soft_wdt_restart();
}

void sj_system_restart() {
  system_restart();
}

int spiffs_get_memory_usage();

size_t sj_get_fs_memory_usage() {
#ifndef V7_NO_FS
  return spiffs_get_memory_usage();
#else
  return 0;
#endif
}

void sj_usleep(int usecs) {
  os_delay_us(usecs);
}

void sj_invoke_cb(struct v7* v7, v7_val_t func, v7_val_t this_obj,
                  v7_val_t args) {
#ifndef RTOS_SDK
  _sj_invoke_cb(v7, func, this_obj, args);
#else
  rtos_dispatch_callback(v7, func, this_obj, args);
#endif
}

void sj_prompt_init_hal(struct v7* v7) {
  (void) v7;
#if !defined(NO_PROMPT)
  uart_process_char = sj_prompt_process_char;
  uart_interrupt_cb = sj_prompt_process_char;
#endif
}
