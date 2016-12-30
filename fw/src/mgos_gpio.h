/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/*
 * GPIO API
 */

#ifndef CS_FW_SRC_MGOS_GPIO_H_
#define CS_FW_SRC_MGOS_GPIO_H_

#include <stdbool.h>

#include "fw/src/mgos_init.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_gpio_mode { MGOS_GPIO_MODE_INPUT = 0, MGOS_GPIO_MODE_OUTPUT = 1 };

enum mgos_gpio_pull_type {
  MGOS_GPIO_PULL_NONE = 0,
  MGOS_GPIO_PULL_UP = 1,
  MGOS_GPIO_PULL_DOWN = 2
};

enum mgos_gpio_int_mode {
  MGOS_GPIO_INT_NONE = 0,
  MGOS_GPIO_INT_EDGE_POS = 1,
  MGOS_GPIO_INT_EDGE_NEG = 2,
  MGOS_GPIO_INT_EDGE_ANY = 3,
  MGOS_GPIO_INT_LEVEL_HI = 4,
  MGOS_GPIO_INT_LEVEL_LO = 5
};

typedef void (*mgos_gpio_int_handler_f)(int pin, void *arg);

/* Set mode - input or output */
bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode);

/* Set pull-up or pull-down type. */
bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull);

/* Read pin input level. */
bool mgos_gpio_read(int pin);

/* Set pin's output level. */
void mgos_gpio_write(int pin, bool level);

/* Flip output pin value. Returns value that was written. */
bool mgos_gpio_toggle(int pin);

/*
 * Install a GPIO interrupt handler.
 *
 * Note that this will not enable the interrupt, this must be done explicitly
 * with mgos_gpio_enable_int.
 */
bool mgos_gpio_set_int_handler(int pin, enum mgos_gpio_int_mode mode,
                               mgos_gpio_int_handler_f cb, void *arg);

/* Enable interrupt on the specified pin. */
bool mgos_gpio_enable_int(int pin);

/* Disables interrupt (without removing the handler). */
bool mgos_gpio_disable_int(int pin);

/*
 * Removes a previosuly set interrupt handler.
 * If cb and arg are not NULL, they will contain previous handler and arg.
 */
void mgos_gpio_remove_int_handler(int pin, mgos_gpio_int_handler_f *old_cb,
                                  void **old_arg);

/*
 * Handle a button on the specified pin.
 * Configures the pin for input with specified pull-up and performs debouncing:
 * upon first triggering user's callback is invoked immediately but further
 * interrupts are inhibited for the following debounce_ms millseconds.
 * Typically 50 ms of debouncing time is sufficient.
 * int_mode is one of the MGOS_GPIO_INT_EDGE_* values and will specify whether
 * the handler triggers when button is pressed, released or both.
 * Which is which depends on how the button is wired: if the normal state is
 * pull-up (typical), then MGOS_GPIO_INT_EDGE_NEG is press and _POS is release.
 *
 * Calling with cb = NULL will remove a previously installed handler.
 *
 * Note: implicitly enables the interrupt.
 */
bool mgos_gpio_set_button_handler(int pin, enum mgos_gpio_pull_type pull_type,
                                  enum mgos_gpio_int_mode int_mode,
                                  int debounce_ms, mgos_gpio_int_handler_f cb,
                                  void *arg);

enum mgos_init_result mgos_gpio_init();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_GPIO_H_ */
