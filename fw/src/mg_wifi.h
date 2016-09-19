/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_WIFI_H_
#define CS_FW_SRC_MG_WIFI_H_

enum mg_wifi_status {
  MG_WIFI_DISCONNECTED = 0,
  MG_WIFI_CONNECTED = 1,
  MG_WIFI_IP_ACQUIRED = 2,
};

typedef void (*mg_wifi_changed_t)(enum mg_wifi_status event, void *arg);
/* Add a callback to be invoked when WiFi state changes. */
void mg_wifi_add_on_change_cb(mg_wifi_changed_t fn, void *arg);
/* Remove a previously added callback, fn and arg have to match exactly. */
void mg_wifi_remove_on_change_cb(mg_wifi_changed_t fn, void *arg);

/* HAL interface, to be implemented by ports. */
struct sys_config_wifi_sta;
struct sys_config_wifi_ap;

int mg_wifi_setup_sta(const struct sys_config_wifi_sta *cfg);

int mg_wifi_setup_ap(const struct sys_config_wifi_ap *cfg);

int mg_wifi_connect(void); /* To previously _setup network. */
int mg_wifi_disconnect(void);

/* These return allocated strings which will be free'd. */
char *mg_wifi_get_status_str(void);
char *mg_wifi_get_connected_ssid(void);
char *mg_wifi_get_sta_ip(void);
char *mg_wifi_get_ap_ip(void);

/*
 * Callback must be invoked, with list of SSIDs or NULL on error.
 * Caller owns SSIDS, they are not freed by the callee.
 * Invoking inline is ok.
 */
typedef void (*mg_wifi_scan_cb_t)(const char **ssids, void *arg);
void mg_wifi_scan(mg_wifi_scan_cb_t cb, void *arg);

/* Invoke this when Wifi connection state changes. */
void mg_wifi_on_change_cb(enum mg_wifi_status event);

enum mg_wifi_status mg_wifi_get_status(void);

void mg_wifi_hal_init(void);

void mg_wifi_init(void);

#endif /* CS_FW_SRC_MG_WIFI_H_ */
