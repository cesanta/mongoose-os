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

/*
 * Implemented on top of the gptimer driver: the legacy timer group driver
 * (driver/timer.h) used by the ESP32/C3 ports was removed in ESP-IDF 6.
 * Requires CONFIG_GPTIMER_CTRL_FUNC_IN_IRAM=y because
 * mgos_hw_timers_dev_clear() is invoked from ISR context for one-shot
 * timers.
 */

#include "esp32_hw_timers.h"

#include <stdint.h>

#include "common/cs_dbg.h"

#include "driver/gptimer.h"

#include "mgos_hw_timers_hal.h"

#define TIMER_RESOLUTION_HZ 1000000 /* 1 tick = 1 us */

IRAM static bool esp32c6_timer_cb(gptimer_handle_t th,
                                  const gptimer_alarm_event_data_t *edata,
                                  void *arg) {
  mgos_hw_timers_isr((struct mgos_hw_timer_info *) arg);
  (void) th;
  (void) edata;
  return false;
}

IRAM bool mgos_hw_timers_dev_set(struct mgos_hw_timer_info *ti, int usecs,
                                 int flags) {
  struct mgos_hw_timer_dev_data *dd = &ti->dev;
  if (dd->started) {
    gptimer_stop(dd->th);
    dd->started = false;
  }
  gptimer_alarm_config_t alarm_cfg = {
      .alarm_count = (uint64_t) usecs,
      .reload_count = 0,
      .flags.auto_reload_on_alarm = ((flags & MGOS_TIMER_REPEAT) != 0),
  };
  if (gptimer_set_raw_count(dd->th, 0) != ESP_OK) return false;
  if (gptimer_set_alarm_action(dd->th, &alarm_cfg) != ESP_OK) return false;
  if (gptimer_start(dd->th) != ESP_OK) return false;
  dd->started = true;
  return true;
}

IRAM void mgos_hw_timers_dev_isr_bottom(struct mgos_hw_timer_info *ti) {
  /* Interrupt status is cleared by the gptimer driver, and for repeating
   * timers auto-reload re-arms the alarm. Nothing to do here. */
  (void) ti;
}

IRAM void mgos_hw_timers_dev_clear(struct mgos_hw_timer_info *ti) {
  struct mgos_hw_timer_dev_data *dd = &ti->dev;
  if (dd->started) {
    gptimer_stop(dd->th);
    dd->started = false;
  }
}

bool mgos_hw_timers_dev_init(struct mgos_hw_timer_info *ti) {
  struct mgos_hw_timer_dev_data *dd = &ti->dev;
  gptimer_config_t cfg = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = TIMER_RESOLUTION_HZ,
  };
  if (gptimer_new_timer(&cfg, &dd->th) != ESP_OK) {
    LOG(LL_ERROR, ("Couldn't allocate HW timer %d", (int) ti->id));
    return false;
  }
  gptimer_event_callbacks_t cbs = {
      .on_alarm = esp32c6_timer_cb,
  };
  if (gptimer_register_event_callbacks(dd->th, &cbs, ti) != ESP_OK ||
      gptimer_enable(dd->th) != ESP_OK) {
    gptimer_del_timer(dd->th);
    dd->th = NULL;
    return false;
  }
  return true;
}
