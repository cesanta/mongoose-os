/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_GPIO_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_GPIO_H_

#include <stdbool.h>
#include <stdint.h>

#include "fw/src/miot_gpio.h"

#define ENTER_CRITICAL(type) ETS_INTR_DISABLE(type)
#define EXIT_CRITICAL(type) ETS_INTR_ENABLE(type)

/* Returns true if next reboot will boot into the boot loader. */
bool esp_strapping_to_bootloader();

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_GPIO_H_ */
