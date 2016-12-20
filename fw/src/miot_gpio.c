/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include "common/cs_dbg.h"

#include "fw/src/miot_gpio.h"
#include "fw/src/miot_gpio_hal.h"
#include "fw/src/miot_hal.h"
#include "fw/src/miot_timers.h"

#ifndef MIOT_NUM_GPIO
#error Please define MIOT_NUM_GPIO
#endif

#ifndef IRAM
#define IRAM
#endif

struct miot_gpio_state {
  miot_gpio_int_handler_f cb;
  void *cb_arg;
  unsigned int cb_pending : 1;
  unsigned int debounce_ms : 16;
};
static struct miot_gpio_state s_state[MIOT_NUM_GPIO];

static void miot_gpio_int_cb(void *arg);
static void miot_gpio_int_done_cb(void *arg);

/* In ISR context */
IRAM void miot_gpio_dev_int_cb(int pin) {
  struct miot_gpio_state *s = &s_state[pin];
  if (s->cb_pending || s->cb == NULL) return;
  if (miot_invoke_cb(miot_gpio_int_cb, (void *) (intptr_t) pin)) {
    s->cb_pending = true;
  } else {
    /*
     * Hopefully it wasn't a level-triggered intr or we'll get into a loop.
     * But what else can we do?
     */
    miot_gpio_dev_int_done(pin);
  }
}

/* In MIOT task context. */
static void miot_gpio_int_cb(void *arg) {
  int pin = (intptr_t) arg;
  struct miot_gpio_state *s = (struct miot_gpio_state *) &s_state[pin];
  if (!s->cb_pending || s->cb == NULL) return;
  s->cb(pin, s->cb_arg);
  if (s->debounce_ms == 0) {
    miot_gpio_int_done_cb(arg);
  } else {
    miot_set_timer(s->debounce_ms, false, miot_gpio_int_done_cb, arg);
  }
}

static void miot_gpio_int_done_cb(void *arg) {
  int pin = (intptr_t) arg;
  struct miot_gpio_state *s = (struct miot_gpio_state *) &s_state[pin];
  if (!s->cb_pending) return;
  s->cb_pending = false;
  miot_gpio_dev_int_done(pin);
}

bool miot_gpio_set_int_handler(int pin, enum miot_gpio_int_mode mode,
                               miot_gpio_int_handler_f cb, void *arg) {
  if (!miot_gpio_dev_set_int_mode(pin, mode)) return false;
  if (mode == MIOT_GPIO_INT_NONE) return true;
  struct miot_gpio_state *s = (struct miot_gpio_state *) &s_state[pin];
  s->cb = cb;
  s->cb_arg = arg;
  s->debounce_ms = 0;
  s->cb_pending = false;
  return true;
}

bool miot_gpio_set_button_handler(int pin, enum miot_gpio_pull_type pull_type,
                                  enum miot_gpio_int_mode int_mode,
                                  int debounce_ms, miot_gpio_int_handler_f cb,
                                  void *arg) {
  if (!miot_gpio_set_mode(pin, MIOT_GPIO_MODE_INPUT) ||
      !miot_gpio_set_pull(pin, pull_type) ||
      !miot_gpio_set_int_handler(pin, int_mode, cb, arg)) {
    return false;
  }
  s_state[pin].debounce_ms = debounce_ms;
  return miot_gpio_enable_int(pin);
}

enum miot_init_result miot_gpio_init() {
  return miot_gpio_dev_init();
}
