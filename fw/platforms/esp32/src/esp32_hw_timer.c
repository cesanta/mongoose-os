/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdint.h>

#include "common/cs_dbg.h"

#include "driver/timer.h"

#include "mgos_hal.h"
#include "mgos_timers.h"

#define NUM_TIMERS 4

struct timer_info {
  timg_dev_t *tg;
  int tn;
  timer_callback cb;
  void *cb_arg;
  int flags;
};

static struct timer_info s_timers[NUM_TIMERS] = {
    {.tg = &TIMERG0, .tn = 0},
    {.tg = &TIMERG0, .tn = 1},
    {.tg = &TIMERG1, .tn = 0},
    {.tg = &TIMERG1, .tn = 1},
};

IRAM void hw_timer_isr(void *arg) {
  struct timer_info *ti = (struct timer_info *) arg;
  ti->cb(ti->cb_arg);
  if (ti->tn == 0) {
    ti->tg->int_clr_timers.t0 = 1;
  } else {
    ti->tg->int_clr_timers.t1 = 1;
  }
  if (ti->flags & MGOS_TIMER_REPEAT) {
    ti->tg->hw_timer[ti->tn].config.alarm_en = 1;
  } else {
    ti->cb_arg = NULL;
    ti->cb = NULL;
  }
}

mgos_timer_id mgos_set_hw_timer(int usecs, int flags, timer_callback cb,
                                void *cb_arg) {
  mgos_timer_id id;
  struct timer_info *ti = NULL;
  mgos_lock();
  for (id = 0; id < NUM_TIMERS; id++) {
    if (s_timers[id].cb == NULL) {
      ti = &s_timers[id];
      break;
    }
  }
  if (ti == NULL) {
    mgos_unlock();
    LOG(LL_ERROR, ("No HW timers available."));
    return MGOS_INVALID_TIMER_ID;
  }
  ti->cb = cb;
  ti->cb_arg = cb_arg;
  ti->flags = flags;
  mgos_unlock();

  int tgn = id / 2;
  timer_config_t config;
  config.alarm_en = TIMER_ALARM_EN;
  config.auto_reload =
      (flags & MGOS_TIMER_REPEAT ? TIMER_AUTORELOAD_EN : TIMER_AUTORELOAD_DIS);
  config.counter_dir = TIMER_COUNT_UP;
  /* Set up divider to tick the timer every 1 uS */
  config.divider = (TIMER_BASE_CLK / 1000000);
  config.intr_type = TIMER_INTR_LEVEL;
  config.counter_en = TIMER_PAUSE;
  if (timer_init(tgn, ti->tn, &config) != ESP_OK) {
    LOG(LL_ERROR, ("failed to init timer %d/%d", tgn, ti->tn));
    ti->cb_arg = NULL;
    ti->cb = NULL;
    return MGOS_INVALID_TIMER_ID;
  }
  timer_set_counter_value(tgn, ti->tn, 0);
  timer_set_alarm_value(tgn, ti->tn, usecs);
  /* Note: ESP_INTR_FLAG_IRAM is not specified, so there's no requirement to pin
   * all the functions used in the ISR to IRAM. This may cause update stalls
   * during flash read/write operations, but is safe. */
  timer_isr_register(tgn, ti->tn, hw_timer_isr, ti, 0, NULL);
  timer_enable_intr(tgn, ti->tn);
  timer_start(tgn, ti->tn);
  return id + 1;
}

IRAM void mgos_clear_hw_timer(mgos_timer_id id) {
  if (id < 1 || id > NUM_TIMERS) return;
  id--;
  int tgn = id / 2;
  struct timer_info *ti = &s_timers[id];
  timer_pause(tgn, ti->tn);
  ti->cb_arg = NULL;
  ti->cb = NULL;
}
