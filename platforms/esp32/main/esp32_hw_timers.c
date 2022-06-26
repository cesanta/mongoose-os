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

#include "esp32_hw_timers.h"

#include <stdint.h>

#include "common/cs_dbg.h"

#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "mgos_hw_timers_hal.h"

IRAM bool mgos_hw_timers_dev_set(struct mgos_hw_timer_info *ti, int usecs,
                                 int flags) {
  struct mgos_hw_timer_dev_data *dd = &ti->dev;
  timg_dev_t *tg = dd->tg;
  int tn = dd->tn;

  tg->hw_timer[tn].config.val =
      (TIMG_T0_INCREASE | TIMG_T0_ALARM_EN | TIMG_T0_LEVEL_INT_EN |
       /* Set up divider to tick the timer every 1 uS */
       ((TIMER_BASE_CLK / 1000000) << TIMG_T0_DIVIDER_S));
  tg->hw_timer[tn].config.autoreload = ((flags & MGOS_TIMER_REPEAT) != 0);

  tg->hw_timer[tn].load_high = 0;
  tg->hw_timer[tn].load_low = 0;
  tg->hw_timer[tn].alarm_high = 0;
  tg->hw_timer[tn].alarm_low = usecs;
  tg->hw_timer[tn].reload = 1;

  /*
   * Note: timer_isr_register is not IRAM safe and this makes the first
   * invocation of mgos_set_hw_timer not IRAM-safe as well.
   * Hopefully this is not a big deal, and the fix is not trivial because of
   * interrupt shortage. Ideally, we'd use a single shared interrupt for all
   * the timers but esp_intr_set_in_iram does not work with shared ints yet.
   */
  uint32_t mask = (1 << tn);
  if (dd->inth == NULL) {
    int intr_source = 0;
    switch (dd->tgn) {
      case TIMER_GROUP_0:
      default:
        intr_source = ETS_TG0_T0_LEVEL_INTR_SOURCE + tn;
        break;
      case TIMER_GROUP_1:
        intr_source = ETS_TG1_T0_LEVEL_INTR_SOURCE + tn;
        break;
    }
    uint32_t status_reg = (uint32_t) &tg->int_st_timers.val;
    if (esp_intr_alloc_intrstatus(intr_source, 0, status_reg, mask,
                                  (void (*)(void *)) mgos_hw_timers_isr, ti,
                                  &dd->inth) != ESP_OK) {
      LOG(LL_ERROR, ("Couldn't allocate into for HW timer"));
      return false;
    }
  }

  bool want_iram = (flags & MGOS_ESP32_HW_TIMER_IRAM) != 0;
  if (want_iram != dd->iram) {
    if (esp_intr_set_in_iram(dd->inth, want_iram) != ESP_OK) return false;
    dd->iram = want_iram;
  }

  tg->int_ena.val |= mask;

  /* Start the timer */
  tg->hw_timer[tn].config.enable = true;

  return true;
}

IRAM void mgos_hw_timers_dev_isr_bottom(struct mgos_hw_timer_info *ti) {
  ti->dev.tg->int_clr_timers.val = (1 << ti->dev.tn);
  if (ti->flags & MGOS_TIMER_REPEAT) {
    ti->dev.tg->hw_timer[ti->dev.tn].config.alarm_en = 1;
  }
}

IRAM void mgos_hw_timers_dev_clear(struct mgos_hw_timer_info *ti) {
  ti->dev.tg->hw_timer[ti->dev.tn].config.enable = 0;
}

bool mgos_hw_timers_dev_init(struct mgos_hw_timer_info *ti) {
  struct mgos_hw_timer_dev_data *dd = &ti->dev;
  switch (ti->id) {
    case 1:
    case 2: {
      dd->tg = &TIMERG0;
      dd->tgn = 0;
      dd->tn = ti->id - 1;
      periph_module_enable(PERIPH_TIMG0_MODULE);
      break;
    }
    case 3:
    case 4: {
      dd->tg = &TIMERG1;
      dd->tgn = 1;
      dd->tn = ti->id - 3;
      periph_module_enable(PERIPH_TIMG1_MODULE);
      break;
    }
    default:
      return false;
  }
  return true;
}
