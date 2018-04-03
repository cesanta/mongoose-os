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

#include <stdio.h>
#include "common/cs_dbg.h"

#include "mgos_gpio_hal.h"
#include "mgos_gpio_internal.h"
#include "mgos_hal.h"
#include "mgos_timers.h"

#ifndef MGOS_NUM_GPIO
#error Please define MGOS_NUM_GPIO
#endif

#ifndef IRAM
#define IRAM
#endif

struct mgos_gpio_state {
  mgos_gpio_int_handler_f cb;
  void *cb_arg;
  unsigned int isr : 1;
  unsigned int cb_pending : 1;
  unsigned int btn_active_state : 1;
  unsigned int debounce_ms : 16;
};
static struct mgos_gpio_state s_state[MGOS_NUM_GPIO];

static void mgos_gpio_int_cb(void *arg);
static void mgos_gpio_dbnc_done_cb(void *arg);

/* In ISR context */
IRAM void mgos_gpio_hal_int_cb(int pin) {
  struct mgos_gpio_state *s = &s_state[pin];
  if (s->cb == NULL) return;
  if (s->isr) {
    s->cb(pin, s->cb_arg);
    mgos_gpio_hal_int_done(pin);
    return;
  }
  if (s->cb_pending) return;
  if (mgos_invoke_cb(mgos_gpio_int_cb, (void *) (intptr_t) pin,
                     true /* from_isr */)) {
    s->cb_pending = true;
  } else {
    /*
     * Hopefully it wasn't a level-triggered intr or we'll get into a loop.
     * But what else can we do?
     */
    mgos_gpio_hal_int_done(pin);
  }
}

/* In MGOS task context. */
static void mgos_gpio_int_cb(void *arg) {
  int pin = (intptr_t) arg;
  struct mgos_gpio_state *s = (struct mgos_gpio_state *) &s_state[pin];
  if (!s->cb_pending || s->cb == NULL) return;
  if (s->debounce_ms == 0) {
    s->cb(pin, s->cb_arg);
    s->cb_pending = false;
    mgos_gpio_hal_int_done(pin);
  } else {
    /* Keep the int disabled for the duration of the debounce time */
    mgos_set_timer(s->debounce_ms, false, mgos_gpio_dbnc_done_cb, arg);
  }
}

static void mgos_gpio_dbnc_done_cb(void *arg) {
  int pin = (intptr_t) arg;
  struct mgos_gpio_state *s = (struct mgos_gpio_state *) &s_state[pin];
  if (mgos_gpio_read(pin) == s->btn_active_state) s->cb(pin, s->cb_arg);
  s->cb_pending = false;
  /* Clear any noise that happened during debounce timer. */
  mgos_gpio_hal_int_clr(pin);
  mgos_gpio_hal_int_done(pin);
}

static bool gpio_set_int_handler_common(int pin, enum mgos_gpio_int_mode mode,
                                        mgos_gpio_int_handler_f cb, void *arg,
                                        bool isr) {
  if (pin < 0 || pin > MGOS_NUM_GPIO) return false;
  if (!mgos_gpio_hal_set_int_mode(pin, mode)) return false;
  if (mode == MGOS_GPIO_INT_NONE) return true;
  struct mgos_gpio_state *s = (struct mgos_gpio_state *) &s_state[pin];
  s->isr = isr;
  s->cb = cb;
  s->cb_arg = arg;
  s->debounce_ms = 0;
  s->cb_pending = false;
  return true;
}

bool mgos_gpio_set_int_handler(int pin, enum mgos_gpio_int_mode mode,
                               mgos_gpio_int_handler_f cb, void *arg) {
  return gpio_set_int_handler_common(pin, mode, cb, arg, false /* isr */);
}

bool mgos_gpio_set_int_handler_isr(int pin, enum mgos_gpio_int_mode mode,
                                   mgos_gpio_int_handler_f cb, void *arg) {
  return gpio_set_int_handler_common(pin, mode, cb, arg, true /* isr */);
}

void mgos_gpio_remove_int_handler(int pin, mgos_gpio_int_handler_f *old_cb,
                                  void **old_arg) {
  mgos_gpio_int_handler_f cb = NULL;
  void *cb_arg = NULL;
  if (pin >= 0 && pin < MGOS_NUM_GPIO) {
    struct mgos_gpio_state *s = (struct mgos_gpio_state *) &s_state[pin];
    mgos_gpio_disable_int(pin);
    cb = s->cb;
    cb_arg = s->cb_arg;
    s->cb = NULL;
    s->cb_arg = NULL;
  }
  if (old_cb != NULL) *old_cb = cb;
  if (old_arg != NULL) *old_arg = cb_arg;
}

bool mgos_gpio_set_button_handler(int pin, enum mgos_gpio_pull_type pull_type,
                                  enum mgos_gpio_int_mode int_mode,
                                  int debounce_ms, mgos_gpio_int_handler_f cb,
                                  void *arg) {
  if (!(int_mode == MGOS_GPIO_INT_EDGE_POS ||
        int_mode == MGOS_GPIO_INT_EDGE_NEG) ||
      !mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_INPUT) ||
      !mgos_gpio_set_pull(pin, pull_type) ||
      !mgos_gpio_set_int_handler(pin, int_mode, cb, arg)) {
    return false;
  }
  s_state[pin].btn_active_state = (int_mode == MGOS_GPIO_INT_EDGE_POS);
  s_state[pin].debounce_ms = debounce_ms;
  return mgos_gpio_enable_int(pin);
}

IRAM bool mgos_gpio_toggle(int pin) {
  bool v = !mgos_gpio_read_out(pin);
  mgos_gpio_write(pin, v);
  return v;
}

enum mgos_init_result mgos_gpio_init() {
  return mgos_gpio_hal_init();
}
