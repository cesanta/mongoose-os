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

#include "mgos.h"

#include "mgos_gpio.h"
#include "mgos_provision.h"

#define LED_ON mgos_sys_config_get_provision_led_active_high()
#define LED_OFF !mgos_sys_config_get_provision_led_active_high()

static void mgos_provision_state_change_cb(int ev, void *evd, void *arg) {
  int pin = mgos_sys_config_get_provision_led_pin();
  if (pin < 0) return;
  const struct mgos_provision_state_changed_arg *ev_arg =
      (struct mgos_provision_state_changed_arg *) evd;
  if (mgos_sys_config_get_provision_stable_state() > 0 &&
      ev_arg->cur_state >= mgos_sys_config_get_provision_stable_state()) {
    mgos_gpio_blink(pin, 0, 0);
    mgos_gpio_write(pin, LED_ON);
    return;
  }
  switch (ev_arg->cur_state) {
    case MGOS_PROVISION_ST_NOT_PROVISIONED: {
      mgos_gpio_blink(pin, 1000, 1000);
      break;
    }
    case MGOS_PROVISION_ST_NETWORK_CONFIGURED: {
      mgos_gpio_blink(pin, 500, 500);
      break;
    }
    case MGOS_PROVISION_ST_NETWORK_CONNECTED: {
      mgos_gpio_blink(pin, 250, 250);
      break;
    }
    case MGOS_PROVISION_ST_CLOUD_CONNECTED: {
      mgos_gpio_blink(pin, 0, 0);
      mgos_gpio_write(pin, LED_ON);
      break;
    }
    default:
      break;
  }
  (void) ev;
  (void) arg;
}

bool mgos_provision_led_init(void) {
  int pin = mgos_sys_config_get_provision_led_pin();
  if (pin >= 0) {
    mgos_gpio_setup_output(pin, LED_OFF);
    mgos_event_add_handler(MGOS_PROVISION_EV_STATE_CHANGED,
                           mgos_provision_state_change_cb, NULL);
  }
  return true;
}
