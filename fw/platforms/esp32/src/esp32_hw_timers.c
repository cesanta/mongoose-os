/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "esp32_hw_timers.h"

#include <stdint.h>

#include "common/cs_dbg.h"

#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "mgos_hw_timers_hal.h"

IRAM bool mgos_hw_timers_dev_set(struct mgos_hw_timer_info *ti, int usecs,
                                 int flags) {
  timg_dev_t *tg = ti->dev.tg;
  int tn = ti->dev.tn;

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

  esp_intr_set_in_iram(ti->dev.inth, (flags & MGOS_ESP32_HW_TIMER_IRAM) != 0);

  tg->int_ena.val |= (1 << tn);

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
  return (timer_isr_register(dd->tgn, dd->tn,
                             (void (*) (void *)) mgos_hw_timers_isr, ti, 0,
                             &dd->inth) == ESP_OK);
}
