#include "esp_missing_includes.h"
#include <v7.h>
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

#ifndef RTOS_SDK
static os_timer_t js_timeout_timer;
#endif

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

void esp_timer_callback(void* arg) {
  v7_val_t* cb = (v7_val_t*) arg;
  sj_invoke_cb0(v7, *cb);
}

void sj_set_timeout(int msecs, v7_val_t* cb) {
#ifndef RTOS_SDK
  /*
   * used to convey the callback to the timer handler _and_ to root
   * the function so that the GC doesn't deallocate it.
   */
  os_timer_disarm(&js_timeout_timer);
  os_timer_setfn(&js_timeout_timer, esp_timer_callback, cb);
  os_timer_arm(&js_timeout_timer, msecs, 0);
#else
  /*
   * TODO RTOS (alashkin):
   * Check if we need to delete timer
   * (free handle)
   */
  xTimerHandle js_timeout_timer =
      xTimerCreate((const signed char*) "js_timeout_timer",
                   msecs / portTICK_RATE_MS, pdFALSE, cb, esp_timer_callback);
  xTimerStart(js_timeout_timer, 0);
#endif
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
