/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_WIFI_H_
#define CS_FW_SRC_MGOS_WIFI_H_

#include "fw/src/mgos_features.h"

#if MGOS_ENABLE_WIFI

#include <stdbool.h>
#include "fw/src/mgos_init.h"
#include "sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_wifi_status {
  MGOS_WIFI_DISCONNECTED = 0,
  MGOS_WIFI_CONNECTED = 1,
  MGOS_WIFI_IP_ACQUIRED = 2,
};

typedef void (*mgos_wifi_changed_t)(enum mgos_wifi_status event, void *arg);
/* Add a callback to be invoked when WiFi state changes. */
void mgos_wifi_add_on_change_cb(mgos_wifi_changed_t fn, void *arg);
/* Remove a previously added callback, fn and arg have to match exactly. */
void mgos_wifi_remove_on_change_cb(mgos_wifi_changed_t fn, void *arg);

bool mgos_wifi_validate_ap_cfg(const struct sys_config_wifi_ap *cfg,
                               char **msg);
bool mgos_wifi_validate_sta_cfg(const struct sys_config_wifi_sta *cfg,
                                char **msg);

/* HAL interface, to be implemented by ports. */
int mgos_wifi_setup_sta(const struct sys_config_wifi_sta *cfg);

int mgos_wifi_setup_ap(const struct sys_config_wifi_ap *cfg);

int mgos_wifi_connect(void); /* To previously _setup network. */
int mgos_wifi_disconnect(void);

/* These return allocated strings which will be free'd. */
char *mgos_wifi_get_status_str(void);
char *mgos_wifi_get_connected_ssid(void);
char *mgos_wifi_get_sta_ip(void);
char *mgos_wifi_get_ap_ip(void);

/*
 * Callback must be invoked, with list of SSIDs or NULL on error.
 * Caller owns SSIDS, they are not freed by the callee.
 * Invoking inline is ok.
 */
typedef void (*mgos_wifi_scan_cb_t)(const char **ssids, void *arg);
void mgos_wifi_scan(mgos_wifi_scan_cb_t cb, void *arg);

/* Invoke this when Wifi connection state changes. */
void mgos_wifi_on_change_cb(enum mgos_wifi_status event);

enum mgos_wifi_status mgos_wifi_get_status(void);

void mgos_wifi_hal_init(void);

enum mgos_init_result mgos_wifi_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MGOS_ENABLE_WIFI */

#endif /* CS_FW_SRC_MGOS_WIFI_H_ */
