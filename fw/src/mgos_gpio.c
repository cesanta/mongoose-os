/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include "common/cs_dbg.h"

#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_gpio_hal.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_timers.h"

#ifndef MGOS_NUM_GPIO
#error Please define MGOS_NUM_GPIO
#endif

#ifndef IRAM
#define IRAM
#endif

struct mgos_gpio_state {
  mgos_gpio_int_handler_f cb;
  void *cb_arg;
  unsigned int cb_pending : 1;
  unsigned int debounce_ms : 16;
};
static struct mgos_gpio_state s_state[MGOS_NUM_GPIO];

static void mgos_gpio_int_cb(void *arg);
static void mgos_gpio_int_done_cb(void *arg);

/* In ISR context */
IRAM void mgos_gpio_dev_int_cb(int pin) {
  struct mgos_gpio_state *s = &s_state[pin];
  if (s->cb_pending || s->cb == NULL) return;
  if (mgos_invoke_cb(mgos_gpio_int_cb, (void *) (intptr_t) pin)) {
    s->cb_pending = true;
  } else {
    /*
     * Hopefully it wasn't a level-triggered intr or we'll get into a loop.
     * But what else can we do?
     */
    mgos_gpio_dev_int_done(pin);
  }
}

/* In MGOS task context. */
static void mgos_gpio_int_cb(void *arg) {
  int pin = (intptr_t) arg;
  struct mgos_gpio_state *s = (struct mgos_gpio_state *) &s_state[pin];
  if (!s->cb_pending || s->cb == NULL) return;
  s->cb(pin, s->cb_arg);
  if (s->debounce_ms == 0) {
    mgos_gpio_int_done_cb(arg);
  } else {
    mgos_set_timer(s->debounce_ms, false, mgos_gpio_int_done_cb, arg);
  }
}

static void mgos_gpio_int_done_cb(void *arg) {
  int pin = (intptr_t) arg;
  struct mgos_gpio_state *s = (struct mgos_gpio_state *) &s_state[pin];
  if (!s->cb_pending) return;
  s->cb_pending = false;
  mgos_gpio_dev_int_done(pin);
}

bool mgos_gpio_set_int_handler(int pin, enum mgos_gpio_int_mode mode,
                               mgos_gpio_int_handler_f cb, void *arg) {
  if (pin < 0 || pin > MGOS_NUM_GPIO) return false;
  if (!mgos_gpio_dev_set_int_mode(pin, mode)) return false;
  if (mode == MGOS_GPIO_INT_NONE) return true;
  struct mgos_gpio_state *s = (struct mgos_gpio_state *) &s_state[pin];
  s->cb = cb;
  s->cb_arg = arg;
  s->debounce_ms = 0;
  s->cb_pending = false;
  return true;
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
  if (!mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_INPUT) ||
      !mgos_gpio_set_pull(pin, pull_type) ||
      !mgos_gpio_set_int_handler(pin, int_mode, cb, arg)) {
    return false;
  }
  s_state[pin].debounce_ms = debounce_ms;
  return mgos_gpio_enable_int(pin);
}

enum mgos_init_result mgos_gpio_init() {
  return mgos_gpio_dev_init();
}
