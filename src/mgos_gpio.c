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

#include "mgos_gpio_hal.h"
#include "mgos_gpio_internal.h"
#include "mgos_system.h"
#include "mgos_timers.h"

#ifndef IRAM
#define IRAM
#endif

struct mgos_gpio_state {
  int pin;
  union {
    struct {
      unsigned int on_ms : 16;
      unsigned int off_ms : 16;
      mgos_timer_id timer_id;
    } blink;
    struct {
      unsigned int debounce_ms : 16;
      unsigned int active_state : 1;
      unsigned int last_state : 1;
      unsigned int any_edge : 1;
      mgos_timer_id timer_id;
    } button;
  };
  mgos_gpio_int_handler_f cb;
  void *cb_arg;
  uint16_t cnt;
  uint8_t isr;
};

static struct mgos_gpio_state *s_state = NULL;
static int s_num_gpio_states = 0;
struct mgos_rlock_type *s_lock = NULL;

static void mgos_gpio_int_cb(void *arg);
static void mgos_gpio_dbnc_done_cb(void *arg);

static IRAM struct mgos_gpio_state *mgos_gpio_get_state(int pin) {
  for (int i = 0; i < s_num_gpio_states; i++) {
    if (s_state[i].pin == pin) return &s_state[i];
  }
  return NULL;
};

static struct mgos_gpio_state *mgos_gpio_get_or_create_state(int pin) {
  struct mgos_gpio_state *s = mgos_gpio_get_state(pin), *old_s = NULL;
  if (s != NULL) return s;
  s = (struct mgos_gpio_state *) calloc(s_num_gpio_states + 1, sizeof(*s));
  if (s == NULL) return NULL;
  /* State may be accessed from ISR. Disable ints for the time
   * we swap the buffer. */
  mgos_ints_disable();
  if (s_state != NULL) {
    memcpy(s, s_state, s_num_gpio_states * sizeof(*s));
  }
  old_s = s_state;
  s_state = s;
  s = &s_state[s_num_gpio_states++];
  s->pin = pin;
  mgos_ints_enable();
  free(old_s);
  return s;
}

/* In ISR context */
IRAM void mgos_gpio_hal_int_cb(int pin) {
  struct mgos_gpio_state *s = mgos_gpio_get_state(pin);
  mgos_gpio_hal_clear_int(pin);
  if (s == NULL || s->cb == NULL) return;
  if (s->isr) {
    s->cb(pin, s->cb_arg);
    return;
  }
  if (s->cnt == 0) {
    if (mgos_invoke_cb(mgos_gpio_int_cb, (void *) (intptr_t) pin,
                       true /* from_isr */)) {
      s->cnt++;
    } else {
      /*
       * Hopefully it wasn't a level-triggered intr or we'll get into a loop.
       * But what else can we do?
       */
    }
  } else {
    s->cnt++;
  }
}

/* In MGOS task context. */
static void mgos_gpio_int_cb(void *arg) {
  int pin = (intptr_t) arg;
  mgos_rlock(s_lock);
  struct mgos_gpio_state *s = mgos_gpio_get_state(pin);
  if (s == NULL || s->cnt == 0 || s->cb == NULL) goto out;
  if (s->button.debounce_ms == 0) {
    while (s->cnt > 0) {
      mgos_runlock(s_lock);
      s->cb(pin, s->cb_arg);
      mgos_rlock(s_lock);
      s->cnt--;
    }
  } else {
    if (s->button.timer_id == MGOS_INVALID_TIMER_ID) {
      s->button.timer_id = mgos_set_timer(s->button.debounce_ms, false,
                                          mgos_gpio_dbnc_done_cb, arg);
    }
  }
out:
  mgos_runlock(s_lock);
}

static void mgos_gpio_dbnc_done_cb(void *arg) {
  int pin = (intptr_t) arg;
  mgos_gpio_int_handler_f cb = NULL;
  void *cb_arg = NULL;
  {
    mgos_rlock(s_lock);
    struct mgos_gpio_state *s = mgos_gpio_get_state(pin);
    if (s != NULL) {
      bool cur_state = mgos_gpio_read(pin);
      s->button.timer_id = MGOS_INVALID_TIMER_ID;
      if ((s->button.any_edge && cur_state != s->button.last_state) ||
          (!s->button.any_edge && cur_state == s->button.active_state)) {
        cb = s->cb;
        cb_arg = s->cb_arg;
        s->button.last_state = cur_state;
      }
      s->cnt = 0;
    }
    mgos_runlock(s_lock);
  }
  if (cb != NULL) {
    cb(pin, cb_arg);
  }
  /* Clear any noise that happened during debounce timer. */
  mgos_gpio_clear_int(pin);
}

static bool gpio_set_int_handler_common(int pin, enum mgos_gpio_int_mode mode,
                                        mgos_gpio_int_handler_f cb, void *arg,
                                        bool isr) {
  bool ret = false;
  if (!mgos_gpio_hal_set_int_mode(pin, mode)) return false;
  if (mode == MGOS_GPIO_INT_NONE) return true;
  {
    mgos_rlock(s_lock);
    struct mgos_gpio_state *s = mgos_gpio_get_or_create_state(pin);
    if (s != NULL) {
      s->isr = isr;
      s->cb = cb;
      s->cb_arg = arg;
      ret = true;
    }
    mgos_runlock(s_lock);
  }
  return ret;
}

bool mgos_gpio_set_int_handler(int pin, enum mgos_gpio_int_mode mode,
                               mgos_gpio_int_handler_f cb, void *arg) {
  return gpio_set_int_handler_common(pin, mode, cb, arg, false /* isr */);
}

bool mgos_gpio_set_int_handler_isr(int pin, enum mgos_gpio_int_mode mode,
                                   mgos_gpio_int_handler_f cb, void *arg) {
  return gpio_set_int_handler_common(pin, mode, cb, arg, true /* isr */);
}

IRAM bool mgos_gpio_enable_int(int pin) {
  return mgos_gpio_hal_enable_int(pin);
}

IRAM bool mgos_gpio_disable_int(int pin) {
  return mgos_gpio_hal_disable_int(pin);
}

IRAM void mgos_gpio_clear_int(int pin) {
  struct mgos_gpio_state *s = mgos_gpio_get_state(pin);
  if (s == NULL) return;
  mgos_gpio_hal_clear_int(pin);
  s->cnt = 0;
}

void mgos_gpio_remove_int_handler(int pin, mgos_gpio_int_handler_f *old_cb,
                                  void **old_arg) {
  mgos_gpio_int_handler_f cb = NULL;
  void *cb_arg = NULL;
  {
    mgos_rlock(s_lock);
    struct mgos_gpio_state *s = mgos_gpio_get_state(pin);
    if (s != NULL) {
      cb = s->cb;
      cb_arg = s->cb_arg;
      s->cb = NULL;
      s->cb_arg = NULL;
      mgos_gpio_disable_int(pin);
    }
    mgos_runlock(s_lock);
  }
  if (old_cb != NULL) *old_cb = cb;
  if (old_arg != NULL) *old_arg = cb_arg;
}

bool mgos_gpio_set_button_handler(int pin, enum mgos_gpio_pull_type pull_type,
                                  enum mgos_gpio_int_mode int_mode,
                                  int debounce_ms, mgos_gpio_int_handler_f cb,
                                  void *arg) {
  if (!(int_mode == MGOS_GPIO_INT_EDGE_POS ||
        int_mode == MGOS_GPIO_INT_EDGE_NEG ||
        int_mode == MGOS_GPIO_INT_EDGE_ANY) ||
      !mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_INPUT) ||
      !mgos_gpio_set_pull(pin, pull_type) ||
      !mgos_gpio_set_int_handler(pin, int_mode, cb, arg)) {
    return false;
  }
  {
    mgos_rlock(s_lock);
    struct mgos_gpio_state *s = mgos_gpio_get_state(pin);
    if (int_mode == MGOS_GPIO_INT_EDGE_ANY) {
      s->button.any_edge = true;
      s->button.last_state = mgos_gpio_read(pin);
    } else {
      s->button.any_edge = false;
      s->button.active_state = (int_mode == MGOS_GPIO_INT_EDGE_POS);
    }
    s->button.debounce_ms = debounce_ms;
    mgos_runlock(s_lock);
  }
  return mgos_gpio_enable_int(pin);
}

IRAM bool mgos_gpio_toggle(int pin) {
  bool v = !mgos_gpio_read_out(pin);
  mgos_gpio_write(pin, v);
  return v;
}

bool mgos_gpio_setup_input(int pin, enum mgos_gpio_pull_type pull) {
  return (mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_INPUT) &&
          mgos_gpio_set_pull(pin, pull));
}

static void mgos_gpio_blink_timer_cb(void *arg) {
  int pin = (intptr_t) arg;
  mgos_rlock(s_lock);
  struct mgos_gpio_state *s = mgos_gpio_get_state(pin);
  if (s != NULL) {
    bool cur = mgos_gpio_toggle(pin);
    if (s->blink.on_ms != s->blink.off_ms) {
      int timeout = (cur ? s->blink.on_ms : s->blink.off_ms);
      s->blink.timer_id = mgos_set_timer(timeout, 0, mgos_gpio_blink_timer_cb,
                                         (void *) (intptr_t) pin);
    }
  }
  mgos_runlock(s_lock);
}

bool mgos_gpio_blink(int pin, int on_ms, int off_ms) {
  bool res = !(on_ms < 0 || off_ms < 0 || on_ms >= 65536 || off_ms >= 65536);
  if (res) {
    mgos_rlock(s_lock);
    struct mgos_gpio_state *s = mgos_gpio_get_or_create_state(pin);
    if (s != NULL) {
      s->blink.on_ms = on_ms;
      s->blink.off_ms = off_ms;
      if (s->blink.timer_id != MGOS_INVALID_TIMER_ID) {
        mgos_clear_timer(s->blink.timer_id);
        s->blink.timer_id = MGOS_INVALID_TIMER_ID;
      }
      if (on_ms != 0 && off_ms != 0) {
        s->blink.timer_id = mgos_set_timer(
            on_ms,
            (on_ms == off_ms ? MGOS_TIMER_REPEAT : 0) | MGOS_TIMER_RUN_NOW,
            mgos_gpio_blink_timer_cb, (void *) (intptr_t) pin);
        res = (s->blink.timer_id != MGOS_INVALID_TIMER_ID);
      }
    } else {
      res = false;
    }
    mgos_runlock(s_lock);
  }
  return res;
}

enum mgos_init_result mgos_gpio_init() {
  s_lock = mgos_rlock_create();
  return mgos_gpio_hal_init();
}
