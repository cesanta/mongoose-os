/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * GPIO API
 */

#ifndef CS_FW_SRC_MIOT_GPIO_H_
#define CS_FW_SRC_MIOT_GPIO_H_

#include <stdbool.h>

#include "fw/src/miot_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum miot_gpio_mode { MIOT_GPIO_MODE_INPUT = 0, MIOT_GPIO_MODE_OUTPUT = 1 };

enum miot_gpio_pull_type {
  MIOT_GPIO_PULL_NONE = 0,
  MIOT_GPIO_PULL_UP = 1,
  MIOT_GPIO_PULL_DOWN = 2
};

enum miot_gpio_int_mode {
  MIOT_GPIO_INT_NONE = 0,
  MIOT_GPIO_INT_EDGE_POS = 1,
  MIOT_GPIO_INT_EDGE_NEG = 2,
  MIOT_GPIO_INT_EDGE_ANY = 3,
  MIOT_GPIO_INT_LEVEL_HI = 4,
  MIOT_GPIO_INT_LEVEL_LO = 5
};

typedef void (*miot_gpio_int_handler_f)(int pin, void *param);

/* Set mode - input or output */
bool miot_gpio_set_mode(int pin, enum miot_gpio_mode mode);

/* Set pull-up or pull-down type. */
bool miot_gpio_set_pull(int pin, enum miot_gpio_pull_type pull);

/* Read pin input level. */
bool miot_gpio_read(int pin);

/* Set pin's output level. */
void miot_gpio_write(int pin, bool level);

/* Flip output pin value. Returns value that was written. */
bool miot_gpio_toggle(int pin);

/*
 * Install a GPIO interrupt handler.
 *
 * Calling with cb = NULL will remove a previously installed handler.
 * Note that this will not enable the interrupt, this must be done explicitly
 * with miot_gpio_enable_int.
 */
bool miot_gpio_set_int_handler(int pin, enum miot_gpio_int_mode mode,
                               miot_gpio_int_handler_f cb, void *arg);

/* Enable interrupt on the specified pin. */
bool miot_gpio_enable_int(int pin);

/* Disables interrupt (without removing the handler). */
bool miot_gpio_disable_int(int pin);

/*
 * Handle a button on the specified pin.
 * Configures the pin for input with specified pull-up and performs debouncing:
 * upon first triggering user's callback is invoked immediately but further
 * interrupts are inhibited for the following debounce_ms millseconds.
 * Typically 50 ms of debouncing time is sufficient.
 * int_mode is one of the MIOT_GPIO_INT_EDGE_* values and will specify whether
 * the handler triggers when button is pressed, released or both.
 * Which is which depends on how the button is wired: if the normal state is
 * pull-up (typical), then MIOT_GPIO_INT_EDGE_NEG is press and _POS is release.
 *
 * Calling with cb = NULL will remove a previously installed handler.
 *
 * Note: implicitly enables the interrupt.
 */
bool miot_gpio_set_button_handler(int pin, enum miot_gpio_pull_type pull_type,
                                  enum miot_gpio_int_mode int_mode,
                                  int debounce_ms, miot_gpio_int_handler_f cb,
                                  void *arg);

enum miot_init_result miot_gpio_init();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_GPIO_H_ */
