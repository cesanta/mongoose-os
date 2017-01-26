/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_FLASH_BYTES_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_FLASH_BYTES_H_

#include "esp_gdb.h"

/*
 * Install an exception handler that emulates 8-bit and 16-bit
 * access to memory mapped flash addresses.
 */
void flash_emul_init(void);

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_FLASH_BYTES_H_ */
