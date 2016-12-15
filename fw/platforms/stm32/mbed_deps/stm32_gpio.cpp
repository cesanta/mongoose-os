/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "mbed.h"
#undef GPIO_MODE_INPUT
#include <map>
#include "fw/src/miot_gpio.h"

typedef std::map<PinName, gpio_t> gpios_map_t;
static gpios_map_t gpios_map;

int miot_gpio_write(int pin, enum gpio_level level) {
  gpios_map_t::iterator gpio_it = gpios_map.find((PinName)pin);
  if (gpio_it == gpios_map.end()) {
    return -1;
  }

  gpio_write(&gpio_it->second, level);

  return 0;
}

int miot_gpio_set_mode(int pin, enum gpio_mode mode, enum gpio_pull_type pull) {
  gpio_t gpio;

  switch(mode) {
    case GPIO_MODE_INOUT:
      gpio_init_inout(&gpio, (PinName)pin, PIN_INPUT, (PinMode)pull, 0);
      break;
    case GPIO_MODE_INPUT:
      gpio_init_in_ex(&gpio, (PinName)pin, (PinMode)pull);
      break;
    case GPIO_MODE_OUTPUT:
      gpio_init_out(&gpio, (PinName)pin);
      break;
    default:
      return -1;
  }

  /* we have to store initialized gpio objects */
  gpios_map.insert(std::make_pair((PinName)pin, gpio));

  return 0;
}

void miot_gpio_intr_init(f_gpio_intr_handler_t cb, void *arg) {
  /* TODO(alex): implement */
  (void) cb;
  (void) arg;
}

int miot_gpio_intr_set(int pin, enum gpio_int_mode type) {
  /* TODO(alex): implement */
  (void) pin;
  (void) type;
  return -1;
}
