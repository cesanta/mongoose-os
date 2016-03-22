/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/platforms/esp8266/esp_missing_includes.h"
#include "v7/v7.h"
#include "smartjs/src/sj_timers.h"
#include "smartjs/src/sj_v7_ext.h"
#include "smartjs/src/sj_hal.h"

#include "v7_esp.h"
#include "smartjs/src/sj_mongoose.h"
#include "smartjs/src/sj_prompt.h"
#include "common/platforms/esp8266/esp_mg_net_if.h"
#include "esp_uart.h"
#include "common/umm_malloc/umm_malloc.h"

#include <osapi.h>
#include <os_type.h>

size_t sj_get_free_heap_size() {
  return system_get_free_heap_size();
}

size_t sj_get_min_free_heap_size() {
#if defined(ESP_UMM_ENABLE)
  return umm_min_free_heap_size();
#else
  /* Not supported */
  return 0;
#endif
}

extern uint32_t soft_wdt_interval;
/* Should be initialized in user_main by calling sj_wdt_set_timeout */
static uint32_t s_saved_soft_wdt_interval;
#define WDT_MAGIC_TIME 500000

void sj_wdt_disable() {
  /*
   * poor's man version: delays wdt for several hours, but
   * technically wdt is not disabled
   */
  s_saved_soft_wdt_interval = soft_wdt_interval;
  soft_wdt_interval = 0xFFFFFFFF;
  system_soft_wdt_restart();
}

void sj_wdt_enable() {
  soft_wdt_interval = s_saved_soft_wdt_interval;
  system_soft_wdt_restart();
}

void sj_wdt_feed() {
  system_soft_wdt_feed();
}

void sj_wdt_set_timeout(int secs) {
  s_saved_soft_wdt_interval = soft_wdt_interval = secs * WDT_MAGIC_TIME;
  system_soft_wdt_restart();
}

void sj_system_restart(int exit_code) {
  (void) exit_code;
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

void sj_invoke_cb(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                  v7_val_t args) {
  mg_dispatch_v7_callback(v7, func, this_obj, args);
}

void sj_prompt_init_hal(struct v7 *v7) {
  (void) v7;
}

void mongoose_schedule_poll() {
  mg_lwip_mgr_schedule_poll(&sj_mgr);
}
