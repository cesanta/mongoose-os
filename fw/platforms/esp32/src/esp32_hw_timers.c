/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "esp32_hw_timers.h"

#include <stdint.h>

#include "common/cs_dbg.h"

#include "driver/timer.h"

#include "mgos_hw_timers_hal.h"

IRAM bool mgos_hw_timers_dev_set(struct mgos_hw_timer_info *ti, int usecs,
                                 int flags) {
  timer_config_t config;
  config.alarm_en = TIMER_ALARM_EN;
  config.auto_reload =
      (flags & MGOS_TIMER_REPEAT ? TIMER_AUTORELOAD_EN : TIMER_AUTORELOAD_DIS);
  config.counter_dir = TIMER_COUNT_UP;
  /* Set up divider to tick the timer every 1 uS */
  config.divider = (TIMER_BASE_CLK / 1000000);
  config.intr_type = TIMER_INTR_LEVEL;
  config.counter_en = TIMER_PAUSE;
  int tgn = ti->dev.tgn, tn = ti->dev.tn;
  if (timer_init(tgn, tn, &config) != ESP_OK) {
    LOG(LL_ERROR, ("failed to init timer %d/%d", tgn, tn));
    ti->cb_arg = NULL;
    ti->cb = NULL;
    return false;
  }
  timer_set_counter_value(tgn, tn, 0);
  timer_set_alarm_value(tgn, tn, usecs);
  int intr_flags = 0;
  if (flags & MGOS_ESP32_HW_TIMER_NMI) intr_flags |= ESP_INTR_FLAG_NMI;
  if (flags & MGOS_ESP32_HW_TIMER_IRAM) intr_flags |= ESP_INTR_FLAG_IRAM;
  timer_isr_register(tgn, tn, (void (*) (void *)) mgos_hw_timers_isr, ti,
                     intr_flags, NULL);
  timer_enable_intr(tgn, tn);
  timer_start(tgn, tn);
  return true;
}

IRAM void mgos_hw_timers_dev_isr_bottom(struct mgos_hw_timer_info *ti) {
  if (ti->dev.tn == 0) {
    ti->dev.tg->int_clr_timers.t0 = 1;
  } else {
    ti->dev.tg->int_clr_timers.t1 = 1;
  }
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
      break;
    }
    case 3:
    case 4: {
      dd->tg = &TIMERG1;
      dd->tgn = 1;
      dd->tn = ti->id - 3;
      break;
    }
    default:
      return false;
  }
  return true;
}
