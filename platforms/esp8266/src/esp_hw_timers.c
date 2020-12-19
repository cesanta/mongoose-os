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

#include "esp_hw_timers.h"

#include "user_interface.h"

#include "common/cs_dbg.h"
#include "esp_missing_includes.h"

#include "mgos_hw_timers_hal.h"

/*
 * ESP8266 has only one hardware timer, FRC1.
 * It is a 23-bit down counter with an optional prescaler (16 or 256).
 */

#define TM_RELOAD BIT(6)
#define TM_ENABLE BIT(7)

#define TIMER_FREQ 80000000
#define TIMER_PRESCALER_1 0
#define TIMER_PRESCALER_16 4
#define TIMER_PRESCALER_256 8
#define TIMER_MIN_LOAD 500
#define TIMER_MAX_LOAD 8000000

static struct mgos_hw_timer_info *s_ti;

IRAM void nmi_isr(void) {
  mgos_hw_timers_isr(s_ti);
}

#define NMI_INT_ENABLE_REG (PERIPHS_DPORT_BASEADDR)
#define FRC1_INT_ENA BIT(1)

IRAM bool mgos_hw_timers_dev_set(struct mgos_hw_timer_info *ti, int usecs,
                                 int flags) {
  uint32_t ctl = TM_ENABLE;
  if (flags & MGOS_TIMER_REPEAT) ctl |= TM_RELOAD;
  uint32_t load = usecs * (TIMER_FREQ / 1000000);
  if (load < TIMER_MAX_LOAD && load >= TIMER_MIN_LOAD) {
    ctl |= TIMER_PRESCALER_1;
  } else if (load % 256 == 0) {
    ctl |= TIMER_PRESCALER_256;
    load /= 256;
  } else if (load % 16 == 0) {
    ctl |= TIMER_PRESCALER_16;
    load /= 16;
  } else {
    load = 0;
  }
  if (load < TIMER_MIN_LOAD) {
    LOG(LL_ERROR, ("Invalid HW timer value %d", usecs));
    return false;
  }
  RTC_REG_WRITE(FRC1_LOAD_ADDRESS, load);
  RTC_CLR_REG_MASK(FRC1_INT_ADDRESS, FRC1_INT_CLR_MASK);
  if (flags & MGOS_ESP8266_HW_TIMER_NMI) {
    /* There's only one timer anyway, this will do. */
    s_ti = ti;
    ETS_FRC_TIMER1_NMI_INTR_ATTACH(nmi_isr);
    SET_PERI_REG_MASK(NMI_INT_ENABLE_REG, FRC1_INT_ENA);
  } else {
    CLEAR_PERI_REG_MASK(NMI_INT_ENABLE_REG, FRC1_INT_ENA);
    ETS_FRC_TIMER1_INTR_ATTACH((ets_isr_t) mgos_hw_timers_isr, ti);
  }
  TM1_EDGE_INT_ENABLE();
  ETS_FRC1_INTR_ENABLE();
  RTC_REG_WRITE(FRC1_CTRL_ADDRESS, ctl);
  return true;
}

IRAM void mgos_hw_timers_dev_isr_bottom(struct mgos_hw_timer_info *ti) {
  /* Nothing further to do, interrupt is cleared for us. */
  (void) ti;
}

IRAM void mgos_hw_timers_dev_clear(struct mgos_hw_timer_info *ti) {
  RTC_CLR_REG_MASK(FRC1_CTRL_ADDRESS, TM_ENABLE);
  (void) ti;
}

bool mgos_hw_timers_dev_init(struct mgos_hw_timer_info *ti) {
  (void) ti;
  return true;
}
