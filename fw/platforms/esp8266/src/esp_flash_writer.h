/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_FLASH_WRITER_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_FLASH_WRITER_H_

#include <stdbool.h>
#include <stdint.h>

#include "common/mg_str.h"

struct esp_flash_write_ctx {
  uint32_t addr;
  uint32_t max_size;
  uint32_t num_erased;
  uint32_t num_written;
};

bool esp_init_flash_write_ctx(struct esp_flash_write_ctx *wctx, uint32_t addr,
                              uint32_t max_size);
int esp_flash_write(struct esp_flash_write_ctx *wctx, const struct mg_str data);

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_FLASH_WRITER_H_ */
