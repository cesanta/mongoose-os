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

#include "cc32xx_hw_timers.h"

#include <stdint.h>

#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_timer.h"
#include "driverlib/interrupt.h"
#include "driverlib/prcm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/timer.h"

#include "common/cs_dbg.h"

#include "mgos_hw_timers_hal.h"

#define TIMER_FREQ 80000000

static void timer_a0_int_handler(void);
static void timer_a1_int_handler(void);
static void timer_a2_int_handler(void);
static void timer_a3_int_handler(void);

static struct mgos_hw_timer_info *s_ti[MGOS_NUM_HW_TIMERS];

static void timer_a0_int_handler(void) {
  mgos_hw_timers_isr(s_ti[0]);
}

static void timer_a1_int_handler(void) {
  mgos_hw_timers_isr(s_ti[1]);
}

static void timer_a2_int_handler(void) {
  mgos_hw_timers_isr(s_ti[2]);
}

static void timer_a3_int_handler(void) {
  mgos_hw_timers_isr(s_ti[3]);
}

bool mgos_hw_timers_dev_set(struct mgos_hw_timer_info *ti, int usecs,
                            int flags) {
  uint32_t load = usecs * (TIMER_FREQ / 1000000);
  uint32_t base = ti->dev.base;
  MAP_PRCMPeripheralClkEnable(ti->dev.periph, PRCM_RUN_MODE_CLK);
  MAP_PRCMPeripheralReset(ti->dev.periph);
  MAP_TimerConfigure(base, (flags & MGOS_TIMER_REPEAT ? TIMER_CFG_PERIODIC
                                                      : TIMER_CFG_ONE_SHOT));
  MAP_TimerPrescaleSet(base, TIMER_A, 0);
  MAP_TimerLoadSet(base, TIMER_A, load);
  s_ti[ti->id - 1] = ti;
  MAP_IntPrioritySet(ti->dev.int_no, INT_PRIORITY_LVL_1);
  MAP_TimerIntRegister(base, TIMER_A, ti->dev.int_handler);
  MAP_TimerIntEnable(base, TIMER_TIMA_TIMEOUT);
  MAP_TimerEnable(base, TIMER_A);
  return true;
}

void mgos_hw_timers_dev_isr_bottom(struct mgos_hw_timer_info *ti) {
  MAP_TimerIntClear(ti->dev.base, TIMER_ICR_TATOCINT);
}

void mgos_hw_timers_dev_clear(struct mgos_hw_timer_info *ti) {
  MAP_TimerDisable(ti->dev.base, TIMER_A);
}

bool mgos_hw_timers_dev_init(struct mgos_hw_timer_info *ti) {
  static const struct mgos_hw_timer_dev_data s_dd[MGOS_NUM_HW_TIMERS] = {
      {
       .base = TIMERA0_BASE,
       .periph = PRCM_TIMERA0,
       .int_no = INT_TIMERA0A,
       .int_handler = timer_a0_int_handler,
      },
      {
       .base = TIMERA1_BASE,
       .periph = PRCM_TIMERA1,
       .int_no = INT_TIMERA1A,
       .int_handler = timer_a1_int_handler,
      },
      {
       .base = TIMERA2_BASE,
       .periph = PRCM_TIMERA2,
       .int_no = INT_TIMERA2A,
       .int_handler = timer_a2_int_handler,
      },
      {
       .base = TIMERA3_BASE,
       .periph = PRCM_TIMERA3,
       .int_no = INT_TIMERA3A,
       .int_handler = timer_a3_int_handler,
      },
  };

  memcpy(&ti->dev, &s_dd[ti->id - 1], sizeof(ti->dev));
  return true;
}
