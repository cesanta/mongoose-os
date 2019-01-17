#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "mgos.h"
#include "mgos_gpio_hal.h"

bool mgos_gpio_set_mode(int pin, enum mgos_gpio_mode mode) {
  return true;

  (void)pin;
  (void)mode;
}

bool mgos_gpio_set_pull(int pin, enum mgos_gpio_pull_type pull) {
  return true;

  (void)pin;
  (void)pull;
}

bool mgos_gpio_read(int pin) {
  return true;
  (void)pin;
}

void mgos_gpio_write(int pin, bool level) {
  return;
  (void)pin;
  (void)level;
}

bool mgos_gpio_toggle(int pin) {
  return true;
  (void)pin;
}

bool mgos_gpio_enable_int(int pin) {
  return true;
  (void)pin;
}

bool mgos_gpio_disable_int(int pin) {
  return true;
  (void)pin;
}

bool mgos_gpio_hal_set_int_mode(int pin, enum mgos_gpio_int_mode mode) {
  return true;
  (void)pin;
  (void)mode;
}

void mgos_gpio_hal_int_cb(int pin) {
  return;
  (void)pin;
}

enum mgos_init_result mgos_gpio_hal_init(void) {
  return true;
}

