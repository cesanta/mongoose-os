/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_PERIPH_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_PERIPH_H_

#include <stdint.h>

struct gpio_info {
  uint8_t pin;
  uint32_t periph;
  uint32_t func;
};

struct gpio_info *get_gpio_info(uint8_t gpio_no);

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_PERIPH_H_ */
