/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#include "mgos.h"

#include "mgos_core_dump.h"
#include "mgos_hal.h"

#include "rs14100_sdk.h"
#include "rsi_wwdt.h"

static bool s_alive = false;

static void rs14100_wdt_int_handler(void) {
  RSI_WWDT_IntrClear();
  if (s_alive) {
    RSI_WWDT_ReStart(WDT);
    s_alive = false;
    return;
  }
  mgos_cd_puts("\r\nWDT timeout\r\n");
  __builtin_trap();  // Executes an illegal instruction.
}

void mgos_wdt_enable(void) {
  RSI_WWDT_Init(WDT);
  rs14100_set_int_handler(NPSS_TO_MCU_WDT_INTR_IRQn, rs14100_wdt_int_handler);
  NVIC_SetPriority(NPSS_TO_MCU_WDT_INTR_IRQn, 0);
  NVIC_EnableIRQ(NPSS_TO_MCU_WDT_INTR_IRQn);
  RSI_WWDT_IntrUnMask();
  s_alive = true;
  RSI_WWDT_Start(WDT);
}

void mgos_wdt_feed(void) {
  // In case it's already in sys reset phase.
  RSI_WWDT_ReStart(WDT);
  s_alive = true;
}

int x = 0;

void mgos_wdt_set_timeout(int secs) {
  uint32_t wdt_cycles = secs * rs14100_get_lf_fsm_clk(), pow;
  // Find power of 2 above wdt_cycles;
  for (pow = 0; pow < 31; pow++) {
    uint32_t wdt_intvl = (1 << pow);
    if (wdt_intvl >= wdt_cycles) break;
  }
  RSI_WWDT_ConfigIntrTimer(WDT, pow);
  RSI_WWDT_ConfigSysRstTimer(WDT, 15);  // ~1 sec after int.
}

void mgos_wdt_disable(void) {
  RSI_WWDT_Disable(WDT);
}
