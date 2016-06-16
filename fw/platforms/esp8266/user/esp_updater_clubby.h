/*
* Copyright (c) 2016 Cesanta Software Limited
* All rights reserved
*/

#ifndef CS_FW_PLATFORMS_ESP8266_USER_ESP_UPDATER_CLUBBY_H_
#define CS_FW_PLATFORMS_ESP8266_USER_ESP_UPDATER_CLUBBY_H_

#include "fw/platforms/esp8266/user/esp_fs.h"

struct v7;
void init_updater_clubby(struct v7 *v7);

extern int s_clubby_upd_status;
extern struct clubby_event *s_clubby_reply;
struct clubby_event *load_clubby_reply(spiffs *fs);

#endif /* CS_FW_PLATFORMS_ESP8266_USER_ESP_UPDATER_CLUBBY_H_ */
