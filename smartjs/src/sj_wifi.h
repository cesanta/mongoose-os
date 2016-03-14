/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_SRC_SJ_WIFI_H_
#define CS_SMARTJS_SRC_SJ_WIFI_H_

#include "v7/v7.h"

#include "sys_config.h"

int sj_wifi_setup_sta(const struct sys_config_wifi_sta *cfg);

int sj_wifi_setup_ap(const struct sys_config_wifi_ap *cfg);

int sj_wifi_connect(); /* To previously _setup network. */
int sj_wifi_disconnect();

/* These return allocated strings which will be free'd. */
char *sj_wifi_get_status_str();
char *sj_wifi_get_connected_ssid();
char *sj_wifi_get_sta_ip();

/* Caller owns SSIDS, they are not freed by the callee. */
typedef void (*sj_wifi_scan_cb_t)(const char **ssids);
int sj_wifi_scan(sj_wifi_scan_cb_t cb);

/* Invoke this when Wifi connection state changes. */
enum sj_wifi_status {
  SJ_WIFI_DISCONNECTED = 0,
  SJ_WIFI_CONNECTED = 1,
  SJ_WIFI_IP_ACQUIRED = 2,
};
void sj_wifi_on_change_callback(enum sj_wifi_status event);

enum sj_wifi_status sj_wifi_get_status();

void sj_wifi_hal_init(struct v7 *v7);

#endif /* CS_SMARTJS_SRC_SJ_WIFI_H_ */
