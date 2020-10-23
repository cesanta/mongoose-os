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

#pragma once

#include <stdbool.h>

#include "common/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

enum mgos_gpio_mode {
  MGOS_GPIO_MODE_INPUT = 0,
  MGOS_GPIO_MODE_OUTPUT = 1,
  MGOS_GPIO_MODE_OUTPUT_OD = 2, /* open-drain output */
};

enum mgos_gpio_pull_type {
  MGOS_GPIO_PULL_NONE = 0, /* floating */
  MGOS_GPIO_PULL_UP = 1,
  MGOS_GPIO_PULL_DOWN = 2,
};

enum mgos_gpio_int_mode {
  MGOS_GPIO_INT_NONE = 0,
  MGOS_GPIO_INT_EDGE_POS = 1, /* positive edge */
  MGOS_GPIO_INT_EDGE_NEG = 2, /* negative edge */
  MGOS_GPIO_INT_EDGE_ANY = 3, /* any edge - positive or negative */
  MGOS_GPIO_INT_LEVEL_HI = 4, /* high voltage level */
  MGOS_GPIO_INT_LEVEL_LO = 5  /* low voltage level */
};

/* GPIO interrupt handler signature. */
typedef void (*mgos_gpio_int_handler_f)(int pin, void *arg);

/* Set mode - input or output. */
bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode);

/* Set pull-up or pull-down type. */
bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull);

/* Sets up a pin as an input and confiures pull-up or pull-down. */
bool mgos_gpio_setup_input(int pin, enum mgos_gpio_pull_type pull);

/*
 * Sets up pin output while avoiding spurious transitions:
 * desired output level is configured first, then mode.
 */
bool mgos_gpio_setup_output(int pin, bool level);

/* Read pin input level. */
bool mgos_gpio_read(int pin);

/* Set pin's output level. */
void mgos_gpio_write(int pin, bool level);

/* Flip output pin value. Returns value that was written. */
bool mgos_gpio_toggle(int pin);

/* Read the value of the output register. */
bool mgos_gpio_read_out(int pin);

/*
 * Install a GPIO interrupt handler.
 *
 * This will invoke handler on the main task, which makes it possible to use
 * any functions but may delay servicing of the interrupt. If lower latency
 * is required, use `mgos_gpio_set_int_handler_isr`, but you'll need to
 * understand the implications, which are platform-specific.
 *
 * Interrupt is automatically cleared once upon triggering.
 * Then it is disabled until the handler gets a chance to run, at which point
 * it is re-enabled. At this point it may re-trigger immediately if the
 * interrupt condition arose again while the handler was pending or running.
 * Handler may use `mgos_gpio_clear_int` to explicitly clear the condition.
 *
 * Note that this will not enable the interrupt, this must be done explicitly
 * with `mgos_gpio_enable_int()`.
 */
bool mgos_gpio_set_int_handler(int pin, enum mgos_gpio_int_mode mode,
                               mgos_gpio_int_handler_f cb, void *arg);

/*
 * Same as mgos_gpio_set_int_handler but invokes handler in ISR context,
 * without the overhead of a context switch. GPIO interrupts are disabled while
 * the handler is running.
 */
bool mgos_gpio_set_int_handler_isr(int pin, enum mgos_gpio_int_mode mode,
                                   mgos_gpio_int_handler_f cb, void *arg);

/* Enable interrupt on the specified pin. */
bool mgos_gpio_enable_int(int pin);

/* Disables interrupt (without removing the handler). */
bool mgos_gpio_disable_int(int pin);

/* Clears a GPIO interrupt flag. */
void mgos_gpio_clear_int(int pin);

/*
 * Removes a previosuly set interrupt handler.
 *
 * If `cb` and `arg` are not NULL, they will contain previous handler and arg.
 */
void mgos_gpio_remove_int_handler(int pin, mgos_gpio_int_handler_f *old_cb,
                                  void **old_arg);

/*
 * Handle a button on the specified pin.
 *
 * Configures the pin for input with specified pull-up and performs debouncing:
 * upon first triggering user's callback is invoked immediately but further
 * interrupts are inhibited for the following debounce_ms millseconds.
 *
 * Typically 50 ms of debouncing time is sufficient.
 * int_mode is one of the `MGOS_GPIO_INT_EDGE_*` values and will specify whether
 * the handler triggers when button is pressed, released or both.
 * Which is which depends on how the button is wired: if the normal state is
 * pull-up (typical), then `MGOS_GPIO_INT_EDGE_NEG` is press and
 * `_POS` is release.
 *
 * Calling with `cb` = NULL will remove a previously installed handler.
 *
 * Note: implicitly enables the interrupt.
 */
bool mgos_gpio_set_button_handler(int pin, enum mgos_gpio_pull_type pull_type,
                                  enum mgos_gpio_int_mode int_mode,
                                  int debounce_ms, mgos_gpio_int_handler_f cb,
                                  void *arg);

/*
 * A utility function that takes care of blinking an LED.
 * The pin must be configured as output first.
 * On (output "1") and off ("0") times are specified in milliseconds.
 * Set to (0, 0) to disable.
 */
bool mgos_gpio_blink(int pin, int on_ms, int off_ms);

/* String representation of pin number.
 * Will return "PA5" or "PK3" for platforms that have port banks. */
const char *mgos_gpio_str(int pin_def, char buf[8]);

#ifdef __cplusplus
}
#endif

/* Platform-specific includes. */
#ifdef RS14100
#include "rs14100_gpio.h"
#endif
#if CS_PLATFORM == CS_P_STM32
#include "stm32_gpio.h"
#endif
