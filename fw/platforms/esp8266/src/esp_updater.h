/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#ifndef CS_FW_PLATFORMS_ESP8266_SRC_ESP_UPDATER_H_
#define CS_FW_PLATFORMS_ESP8266_SRC_ESP_UPDATER_H_

#include "common/platforms/esp8266/rboot/rboot/appcode/rboot-api.h"

#ifdef __cplusplus
extern "C" {
#endif

rboot_config *get_rboot_config(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_ESP8266_SRC_ESP_UPDATER_H_ */
