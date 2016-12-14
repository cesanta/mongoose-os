/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "freertos/FreeRTOS.h"

#include "esp_system.h"
#include "esp_task_wdt.h"

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"

#include "fw/src/miot_hal.h"
#include "fw/src/miot_sys_config.h"

size_t miot_get_free_heap_size(void) {
  return xPortGetFreeHeapSize();
}

size_t miot_get_min_free_heap_size(void) {
  return xPortGetMinimumEverFreeHeapSize();
}

void miot_system_restart(int exit_code) {
  (void) exit_code;
  esp_restart();
}

void device_get_mac_address(uint8_t mac[6]) {
  esp_efuse_read_mac(mac);
}

void miot_wdt_feed(void) {
  esp_task_wdt_feed();
}

void miot_wdt_disable(void) {
  esp_task_wdt_delete();
}

void miot_wdt_enable(void) {
  /* Feeding the dog re-adds it back to the list if needed. */
  esp_task_wdt_feed();
}

void miot_wdt_set_timeout(int secs) {
  /* WDT0 is configured in esp_task_wdt_init, we only modify the timeout. */
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_config2 = secs * 2000; /* Units: 0.5 ms ticks */
  TIMERG0.wdt_config3 = (secs + 1) * 2000;
  TIMERG0.wdt_config0.en = 1;
  TIMERG0.wdt_feed = 1;
  TIMERG0.wdt_wprotect = 0;
}
