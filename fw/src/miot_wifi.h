/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_WIFI_H_
#define CS_FW_SRC_MIOT_WIFI_H_

#include "fw/src/miot_features.h"

#if MIOT_ENABLE_WIFI

#include <stdbool.h>
#include "fw/src/miot_init.h"
#include "sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum miot_wifi_status {
  MIOT_WIFI_DISCONNECTED = 0,
  MIOT_WIFI_CONNECTED = 1,
  MIOT_WIFI_IP_ACQUIRED = 2,
};

typedef void (*miot_wifi_changed_t)(enum miot_wifi_status event, void *arg);
/* Add a callback to be invoked when WiFi state changes. */
void miot_wifi_add_on_change_cb(miot_wifi_changed_t fn, void *arg);
/* Remove a previously added callback, fn and arg have to match exactly. */
void miot_wifi_remove_on_change_cb(miot_wifi_changed_t fn, void *arg);

bool miot_wifi_validate_ap_cfg(const struct sys_config_wifi_ap *cfg,
                               char **msg);
bool miot_wifi_validate_sta_cfg(const struct sys_config_wifi_sta *cfg,
                                char **msg);

/* HAL interface, to be implemented by ports. */
int miot_wifi_setup_sta(const struct sys_config_wifi_sta *cfg);

int miot_wifi_setup_ap(const struct sys_config_wifi_ap *cfg);

int miot_wifi_connect(void); /* To previously _setup network. */
int miot_wifi_disconnect(void);

/* These return allocated strings which will be free'd. */
char *miot_wifi_get_status_str(void);
char *miot_wifi_get_connected_ssid(void);
char *miot_wifi_get_sta_ip(void);
char *miot_wifi_get_ap_ip(void);

/*
 * Callback must be invoked, with list of SSIDs or NULL on error.
 * Caller owns SSIDS, they are not freed by the callee.
 * Invoking inline is ok.
 */
typedef void (*miot_wifi_scan_cb_t)(const char **ssids, void *arg);
void miot_wifi_scan(miot_wifi_scan_cb_t cb, void *arg);

/* Invoke this when Wifi connection state changes. */
void miot_wifi_on_change_cb(enum miot_wifi_status event);

enum miot_wifi_status miot_wifi_get_status(void);

void miot_wifi_hal_init(void);

enum miot_init_result miot_wifi_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MIOT_ENABLE_WIFI */

#endif /* CS_FW_SRC_MIOT_WIFI_H_ */
