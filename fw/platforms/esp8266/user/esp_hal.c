/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "common/platforms/esp8266/esp_missing_includes.h"
#include "fw/src/mg_timers.h"
#include "fw/src/mg_v7_ext.h"
#include "fw/src/mg_hal.h"

#include "v7_esp.h"
#include "fw/src/mg_mongoose.h"
#include "fw/src/mg_prompt.h"
#include "common/platforms/esp8266/esp_mg_net_if.h"
#include "common/umm_malloc/umm_malloc.h"

#include "fw/platforms/esp8266/user/esp_fs.h"

#ifdef MG_ENABLE_JS
#include "v7/v7.h"
#endif

#include <osapi.h>
#include <os_type.h>

size_t mg_get_heap_size(void) {
  return UMM_MALLOC_CFG__HEAP_SIZE;
}

size_t mg_get_free_heap_size(void) {
  return umm_free_heap_size();
}

size_t mg_get_min_free_heap_size(void) {
  return umm_min_free_heap_size();
}

extern uint32_t soft_wdt_interval;
/* Should be initialized in user_main by calling mg_wdt_set_timeout */
static uint32_t s_saved_soft_wdt_interval;
#define WDT_MAGIC_TIME 500000

void mg_wdt_disable(void) {
  /*
   * poor's man version: delays wdt for several hours, but
   * technically wdt is not disabled
   */
  s_saved_soft_wdt_interval = soft_wdt_interval;
  soft_wdt_interval = 0xFFFFFFFF;
  system_soft_wdt_restart();
}

void mg_wdt_enable(void) {
  soft_wdt_interval = s_saved_soft_wdt_interval;
  system_soft_wdt_restart();
}

void mg_wdt_feed(void) {
  system_soft_wdt_feed();
}

void mg_wdt_set_timeout(int secs) {
  s_saved_soft_wdt_interval = soft_wdt_interval = secs * WDT_MAGIC_TIME;
  system_soft_wdt_restart();
}

void mg_system_restart(int exit_code) {
  (void) exit_code;
  fs_umount();
  system_restart();
}

int spiffs_get_memory_usage();

size_t mg_get_fs_memory_usage(void) {
#ifndef V7_NO_FS
  return spiffs_get_memory_usage();
#else
  return 0;
#endif
}

void mg_usleep(int usecs) {
  os_delay_us(usecs);
}

IRAM void mongoose_schedule_poll(void) {
  mg_lwip_mgr_schedule_poll(mg_get_mgr());
}

#ifdef MG_ENABLE_JS
void mg_invoke_cb(struct v7 *v7, v7_val_t func, v7_val_t this_obj,
                  v7_val_t args) {
  mg_dispatch_v7_callback(v7, func, this_obj, args);
}

void mg_prompt_init_hal(void) {
}
#endif
