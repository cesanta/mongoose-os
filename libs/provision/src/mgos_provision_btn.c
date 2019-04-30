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

#include "mgos_provision.h"

#include "mgos.h"
#include "mgos_event.h"
#include "mgos_gpio.h"
#include "mgos_timers.h"

static mgos_timer_id s_hold_timer = MGOS_INVALID_TIMER_ID;

void mgos_provision_reset(void) {
  LOG(LL_INFO, ("Resetting to factory defaults"));
  mgos_config_reset(MGOS_CONFIG_LEVEL_USER);
  mgos_event_trigger(MGOS_PROVISION_EV_RESET, NULL);
  mgos_fs_gc();
  mgos_system_restart_after(100);
}

static void button_timer_cb(void *arg) {
  int pin = mgos_sys_config_get_provision_btn_pin();
  int hold = mgos_sys_config_get_provision_btn_hold_ms();
  enum mgos_gpio_pull_type pull =
      (mgos_sys_config_get_provision_btn_pull_up() ? MGOS_GPIO_PULL_UP
                                                   : MGOS_GPIO_PULL_DOWN);
  int n = 0; /* Number of times the button is reported down */
  for (int i = 0; i < 10; i++) {
    int level = mgos_gpio_read(pin);
    if (pull == MGOS_GPIO_PULL_UP && level == 0) n++;
    if (pull == MGOS_GPIO_PULL_DOWN && level > 0) n++;
    mgos_msleep(1);
  }
  if (s_hold_timer != MGOS_INVALID_TIMER_ID) mgos_clear_timer(s_hold_timer);
  if (n > 7) mgos_provision_reset();
  if (hold > 0) mgos_gpio_enable_int(pin);
  (void) arg;
}

static void button_down_cb(int pin, void *arg) {
  int duration = mgos_sys_config_get_provision_btn_hold_ms();
  if (s_hold_timer != MGOS_INVALID_TIMER_ID) mgos_clear_timer(s_hold_timer);
  LOG(LL_INFO, ("Button pressed, setting %d ms timer", duration));
  s_hold_timer = mgos_set_timer(duration, 0, button_timer_cb, arg);
  (void) pin;
}

bool mgos_provision_btn_init(void) {
  char buf[8];
  int pin = mgos_sys_config_get_provision_btn_pin();
  int hold = mgos_sys_config_get_provision_btn_hold_ms();
  enum mgos_gpio_pull_type pull =
      (mgos_sys_config_get_provision_btn_pull_up() ? MGOS_GPIO_PULL_UP
                                                   : MGOS_GPIO_PULL_DOWN);

  if (pin < 0 || hold < 0) return true; /* disabled */

  LOG(LL_INFO,
      ("Factory reset button: pin %s, pull %s, hold_ms %d (%s)",
       mgos_gpio_str(pin, buf), (pull == MGOS_GPIO_PULL_UP ? "up" : "down"),
       hold, hold == 0 ? "hold on boot" : "long press"));

  mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_INPUT);
  mgos_gpio_set_pull(pin, pull);

  if (hold == 0) {
    /* Check if button is pressed on reboot */
    button_timer_cb(NULL);
  } else {
    /* Set a long press handler. Note: user code can override it! */
    mgos_gpio_set_button_handler(pin, pull, pull == MGOS_GPIO_PULL_UP
                                                ? MGOS_GPIO_INT_EDGE_NEG
                                                : MGOS_GPIO_INT_EDGE_POS,
                                 50, button_down_cb, NULL);
  }

  return true;
}
