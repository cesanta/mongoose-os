/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_ESP8266_USER_V7_ESP_H_
#define CS_FW_PLATFORMS_ESP8266_USER_V7_ESP_H_

#include <user_interface.h>

#define V7_DEV_CONF_BASE ((char *) 0x40210000)
#define V7_DEV_CONF_SHA1 V7_DEV_CONF_BASE
#define V7_DEV_CONF_STR (V7_DEV_CONF_BASE + 20)

struct v7;

extern struct v7 *v7;

void init_v7(void *dummy);
void run_init_script(void);
void wifi_changed_cb(System_Event_t *evt);
void pp_soft_wdt_restart(void);

#endif /* CS_FW_PLATFORMS_ESP8266_USER_V7_ESP_H_ */
