/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_PLATFORMS_ESP8266_USER_ESP_GPIO_H_
#define CS_SMARTJS_PLATFORMS_ESP8266_USER_ESP_GPIO_H_

#include <stdint.h>

#include "smartjs/src/sj_gpio.h"

#define ENTER_CRITICAL(type) ETS_INTR_DISABLE(type)
#define EXIT_CRITICAL(type) ETS_INTR_ENABLE(type)

int sj_gpio_set_mode(int pin, enum gpio_mode mode, enum gpio_pull_type pull);
int sj_gpio_write(int pin, enum gpio_level level);

#endif /* CS_SMARTJS_PLATFORMS_ESP8266_USER_ESP_GPIO_H_ */
