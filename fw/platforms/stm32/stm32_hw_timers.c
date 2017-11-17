/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "mgos_hw_timers_hal.h"

bool mgos_hw_timers_dev_set(struct mgos_hw_timer_info *ti, int usecs,
                            int flags) {
  /* TODO(rojer) */
  (void) ti;
  (void) usecs;
  (void) flags;
  return false;
}

void mgos_hw_timers_dev_isr_bottom(struct mgos_hw_timer_info *ti) {
  (void) ti;
}

void mgos_hw_timers_dev_clear(struct mgos_hw_timer_info *ti) {
  /* TODO(rojer) */
  (void) ti;
}

bool mgos_hw_timers_dev_init(struct mgos_hw_timer_info *ti) {
  (void) ti;
  return true;
}
