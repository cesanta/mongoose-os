/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_COREDUMP_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_COREDUMP_H_

#ifdef __cplusplus
extern "C" {
#endif

void esp_dump_core(int cause, struct regfile *);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_COREDUMP_H_ */
