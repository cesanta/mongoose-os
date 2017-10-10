/*
 * Generated file - do not edit.
 * Command: ../../fw/tools/gen_sys_config.py --c_name=sys_conf --dest_dir=.build data/sys_conf_wifi.yaml data/sys_conf_http.yaml data/sys_conf_debug.yaml
 */

#ifndef SYS_CONF_H_
#define SYS_CONF_H_

#include "mgos_config_util.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct sys_conf_wifi_sta {
  char *ssid;
  char *pass;
};

struct sys_conf_wifi_ap {
  char *ssid;
  char *pass;
  int channel;
  char *dhcp_end;
};

struct sys_conf_wifi {
  struct sys_conf_wifi_sta sta;
  struct sys_conf_wifi_ap ap;
};

struct sys_conf_http {
  int enable;
  int port;
};

struct sys_conf_debug {
  int level;
  char *dest;
};

struct sys_conf {
  struct sys_conf_wifi wifi;
  struct sys_conf_http http;
  struct sys_conf_debug debug;
};

/* Parametrized accessor prototypes {{{ */
const struct sys_conf_wifi *sys_conf_get_wifi(struct sys_conf *cfg);
const struct sys_conf_wifi_sta *sys_conf_get_wifi_sta(struct sys_conf *cfg);
const char *sys_conf_get_wifi_sta_ssid(struct sys_conf *cfg);
const char *sys_conf_get_wifi_sta_pass(struct sys_conf *cfg);
const struct sys_conf_wifi_ap *sys_conf_get_wifi_ap(struct sys_conf *cfg);
const char *sys_conf_get_wifi_ap_ssid(struct sys_conf *cfg);
const char *sys_conf_get_wifi_ap_pass(struct sys_conf *cfg);
int         sys_conf_get_wifi_ap_channel(struct sys_conf *cfg);
const char *sys_conf_get_wifi_ap_dhcp_end(struct sys_conf *cfg);
const struct sys_conf_http *sys_conf_get_http(struct sys_conf *cfg);
int         sys_conf_get_http_enable(struct sys_conf *cfg);
int         sys_conf_get_http_port(struct sys_conf *cfg);
const struct sys_conf_debug *sys_conf_get_debug(struct sys_conf *cfg);
int         sys_conf_get_debug_level(struct sys_conf *cfg);
const char *sys_conf_get_debug_dest(struct sys_conf *cfg);

void sys_conf_set_wifi_sta_ssid(struct sys_conf *cfg, const char *val);
void sys_conf_set_wifi_sta_pass(struct sys_conf *cfg, const char *val);
void sys_conf_set_wifi_ap_ssid(struct sys_conf *cfg, const char *val);
void sys_conf_set_wifi_ap_pass(struct sys_conf *cfg, const char *val);
void sys_conf_set_wifi_ap_channel(struct sys_conf *cfg, int         val);
void sys_conf_set_wifi_ap_dhcp_end(struct sys_conf *cfg, const char *val);
void sys_conf_set_http_enable(struct sys_conf *cfg, int         val);
void sys_conf_set_http_port(struct sys_conf *cfg, int         val);
void sys_conf_set_debug_level(struct sys_conf *cfg, int         val);
void sys_conf_set_debug_dest(struct sys_conf *cfg, const char *val);
/* }}} */


const struct mgos_conf_entry *sys_conf_schema();

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* SYS_CONF_H_ */
