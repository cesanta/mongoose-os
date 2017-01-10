#include "fw/src/mgos_gpio_hal.h"

enum mgos_init_result mgos_gpio_dev_init(void) {
  /* TODO(alashkin): implement */
  return MGOS_INIT_OK;
}

bool mgos_gpio_toggle(int pin) {
  /* TODO(alashkin): implement */
  (void) pin;
}

bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  /* TODO(alashkin): implement */
  (void) pin;
  (void) mode;
}

bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  /* TODO(alashkin): implement */
  (void) pin;
  (void) pull;
}

void mgos_gpio_dev_int_done(int pin) {
  /* TODO(alashkin): implement */
  (void) pin;
}

bool mgos_gpio_dev_set_int_mode(int pin, enum mgos_gpio_int_mode mode) {
  /* TODO(alashkin): implement */
  (void) pin;
  (void) mode;
}

bool mgos_gpio_enable_int(int pin) {
  /* TODO(alashkin): implement */
  (void) pin;
}

bool mgos_gpio_disable_int(int pin) {
  /* TODO(alashkin): implement */
  (void) pin;
}
