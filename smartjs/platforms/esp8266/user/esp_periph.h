/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef ESP_PERIPH_INCLUDED
#define ESP_PERIPH_INCLUDED

#include <stdint.h>

struct gpio_info {
  uint8_t pin;
  uint32_t periph;
  uint32_t func;
};

struct gpio_info *get_gpio_info(uint8_t gpio_no);

#endif /* ESP_PERIPH_INCLUDED */
