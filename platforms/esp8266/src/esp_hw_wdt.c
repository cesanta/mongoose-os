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

#include "esp_hw_wdt.h"

#include "esp_exc.h"

#ifdef RTOS_SDK
#include <esp_common.h>
#else
#include <user_interface.h>
#endif

#include "esp_hw_wdt_register.h"
#include "esp_missing_includes.h"

extern void esp_hw_wdt_isr(void *arg);

#ifndef ETS_WDT_INUM
#define ETS_WDT_INUM 8
#endif

void esp_hw_wdt_setup(enum esp_hw_wdt_timeout stage0_timeout,
                      enum esp_hw_wdt_timeout stage1_timeout) {
  uint32_t ctl = (WDT_CTL_UNK3 | WDT_CTL_UNK4 | WDT_CTL_UNK5);
  WRITE_PERI_REG(WDT_CTL, ctl);
  if (stage0_timeout == ESP_HW_WDT_DISABLE) {
    return;
  }
  WRITE_PERI_REG(WDT_RELOAD_STAGE0, stage0_timeout);
  if (stage1_timeout != ESP_HW_WDT_DISABLE) {
    WRITE_PERI_REG(WDT_RELOAD_STAGE1, stage1_timeout);
  } else {
    ctl |= WDT_CTL_STAGE1_DISABLE;
    WRITE_PERI_REG(WDT_CTL, ctl);
  }
  SET_PERI_REG_MASK(EDGE_INT_ENABLE_REG, BIT(0));
#ifdef RTOS_SDK
  _xt_isr_attach(ETS_WDT_INUM, esp_hw_wdt_isr, NULL);
  _xt_isr_unmask(1 << ETS_WDT_INUM);
#else
  ets_isr_attach(ETS_WDT_INUM, esp_hw_wdt_isr, NULL);
  ets_isr_unmask(1 << ETS_WDT_INUM);
#endif
  ctl |= WDT_CTL_ENABLE;
  WRITE_PERI_REG(WDT_CTL, ctl);
}

enum esp_hw_wdt_timeout esp_hw_wdt_secs_to_timeout(int secs) {
  enum esp_hw_wdt_timeout timeout;
  if (secs <= 0) {
    timeout = ESP_HW_WDT_DISABLE;
  } else if (secs < 1) {
    timeout = ESP_HW_WDT_0_84_SEC;
  } else if (secs <= 2) {
    timeout = ESP_HW_WDT_1_68_SEC;
  } else if (secs <= 4) {
    timeout = ESP_HW_WDT_3_36_SEC;
  } else if (secs <= 7) {
    timeout = ESP_HW_WDT_6_71_SEC;
  } else if (secs <= 14) {
    timeout = ESP_HW_WDT_13_4_SEC;
  } else {
    timeout = ESP_HW_WDT_26_8_SEC;
  }
  return timeout;
}

void esp_hw_wdt_enable() {
  SET_PERI_REG_MASK(WDT_CTL, WDT_CTL_ENABLE);
}

void esp_hw_wdt_disable() {
  CLEAR_PERI_REG_MASK(WDT_CTL, WDT_CTL_ENABLE);
}

void esp_hw_wdt_feed() {
  WRITE_PERI_REG(WDT_RESET, WDT_RESET_VALUE);
}
