/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "mbed.h"
#include <map>
#include "fw/src/miot_gpio.h"
#include "fw/src/miot_gpio_hal.h"

typedef std::map<PinName, gpio_t> gpios_map_t;
static gpios_map_t gpios_map;

void miot_gpio_write(int pin, bool level) {
  gpios_map_t::iterator gpio_it = gpios_map.find((PinName) pin);
  if (gpio_it == gpios_map.end()) {
    return;
  }

  gpio_write(&gpio_it->second, level);
}

bool miot_gpio_read(int pin) {
  gpios_map_t::iterator gpio_it = gpios_map.find((PinName) pin);
  if (gpio_it == gpios_map.end()) {
    return false;
  }

  return (gpio_read(&gpio_it->second) == 1);
}

bool miot_gpio_toggle(int pin) {
  bool new_value = !miot_gpio_read(pin);
  miot_gpio_write(pin, new_value);

  return new_value;
}

bool miot_gpio_set_mode(int pin, enum miot_gpio_mode mode) {
  gpio_t gpio;

  switch (mode) {
    case MIOT_GPIO_MODE_INPUT:
      gpio_init_in(&gpio, (PinName) pin);
      break;
    case MIOT_GPIO_MODE_OUTPUT:
      gpio_init_out(&gpio, (PinName) pin);
      break;
    default:
      return false;
  }

  /* we have to store initialized gpio objects */
  gpios_map.insert(std::make_pair((PinName) pin, gpio));

  return true;
}

bool miot_gpio_enable_int(int pin) {
  /* TODO(alashkin) */
  (void) pin;
  return false;
}

bool miot_gpio_disable_int(int pin) {
  /* TODO(alashkin) */
  (void) pin;
  return false;
}

bool miot_gpio_set_pull(int pin, enum miot_gpio_pull_type pull) {
  gpio_t gpio;

  /*
   * rojer: Not sure if we need to find previously defined object and call
   * gpio_mode on it?
   * TODO(alashkin): Test.
   */

  switch (pull) {
    case MIOT_GPIO_PULL_NONE:
      gpio_init_in_ex(&gpio, (PinName) pin, PullNone);
      break;
    case MIOT_GPIO_PULL_UP:
      gpio_init_in_ex(&gpio, (PinName) pin, PullUp);
      break;
    case MIOT_GPIO_PULL_DOWN:
      gpio_init_in_ex(&gpio, (PinName) pin, PullDown);
      break;
    default:
      return false;
  }

  /* we have to store initialized gpio objects */
  gpios_map.insert(std::make_pair((PinName) pin, gpio));

  return true;
}

bool miot_gpio_dev_set_int_mode(int pin, enum miot_gpio_int_mode mode) {
  /* TODO(alashkin) */
  return false;
}

enum miot_init_result miot_gpio_dev_init(void) {
  return MIOT_INIT_OK;
}
