/* clang-format off */
/*
 * Generated file - do not edit.
 * Command: ../../fw/tools/gen_sys_config.py --c_name=sys_conf --dest_dir=.build data/sys_conf_wifi.yaml data/sys_conf_http.yaml data/sys_conf_debug.yaml data/sys_conf_overrides.yaml
 */

#include <stddef.h>
#include "sys_conf.h"

const struct mgos_conf_entry sys_conf_schema_[19] = {
  {.type = CONF_TYPE_OBJECT, .key = "", .offset = 0, .num_desc = 18},
  {.type = CONF_TYPE_OBJECT, .key = "wifi", .offset = offsetof(struct sys_conf, wifi), .num_desc = 8},
  {.type = CONF_TYPE_OBJECT, .key = "sta", .offset = offsetof(struct sys_conf, wifi.sta), .num_desc = 2},
  {.type = CONF_TYPE_STRING, .key = "ssid", .offset = offsetof(struct sys_conf, wifi.sta.ssid)},
  {.type = CONF_TYPE_STRING, .key = "pass", .offset = offsetof(struct sys_conf, wifi.sta.pass)},
  {.type = CONF_TYPE_OBJECT, .key = "ap", .offset = offsetof(struct sys_conf, wifi.ap), .num_desc = 4},
  {.type = CONF_TYPE_STRING, .key = "ssid", .offset = offsetof(struct sys_conf, wifi.ap.ssid)},
  {.type = CONF_TYPE_STRING, .key = "pass", .offset = offsetof(struct sys_conf, wifi.ap.pass)},
  {.type = CONF_TYPE_INT, .key = "channel", .offset = offsetof(struct sys_conf, wifi.ap.channel)},
  {.type = CONF_TYPE_STRING, .key = "dhcp_end", .offset = offsetof(struct sys_conf, wifi.ap.dhcp_end)},
  {.type = CONF_TYPE_INT, .key = "foo", .offset = offsetof(struct sys_conf, foo)},
  {.type = CONF_TYPE_OBJECT, .key = "http", .offset = offsetof(struct sys_conf, http), .num_desc = 2},
  {.type = CONF_TYPE_BOOL, .key = "enable", .offset = offsetof(struct sys_conf, http.enable)},
  {.type = CONF_TYPE_INT, .key = "port", .offset = offsetof(struct sys_conf, http.port)},
  {.type = CONF_TYPE_OBJECT, .key = "debug", .offset = offsetof(struct sys_conf, debug), .num_desc = 4},
  {.type = CONF_TYPE_INT, .key = "level", .offset = offsetof(struct sys_conf, debug.level)},
  {.type = CONF_TYPE_STRING, .key = "dest", .offset = offsetof(struct sys_conf, debug.dest)},
  {.type = CONF_TYPE_DOUBLE, .key = "test_d1", .offset = offsetof(struct sys_conf, debug.test_d1)},
  {.type = CONF_TYPE_DOUBLE, .key = "test_d2", .offset = offsetof(struct sys_conf, debug.test_d2)},
};

const struct mgos_conf_entry *sys_conf_schema() {
  return sys_conf_schema_;
}

/* Getters {{{ */
const struct sys_conf_wifi *sys_conf_get_wifi(struct sys_conf *cfg) {
  return &cfg->wifi;
}
const struct sys_conf_wifi_sta *sys_conf_get_wifi_sta(struct sys_conf *cfg) {
  return &cfg->wifi.sta;
}
const char *sys_conf_get_wifi_sta_ssid(struct sys_conf *cfg) {
  return cfg->wifi.sta.ssid;
}
const char *sys_conf_get_wifi_sta_pass(struct sys_conf *cfg) {
  return cfg->wifi.sta.pass;
}
const struct sys_conf_wifi_ap *sys_conf_get_wifi_ap(struct sys_conf *cfg) {
  return &cfg->wifi.ap;
}
const char *sys_conf_get_wifi_ap_ssid(struct sys_conf *cfg) {
  return cfg->wifi.ap.ssid;
}
const char *sys_conf_get_wifi_ap_pass(struct sys_conf *cfg) {
  return cfg->wifi.ap.pass;
}
int         sys_conf_get_wifi_ap_channel(struct sys_conf *cfg) {
  return cfg->wifi.ap.channel;
}
const char *sys_conf_get_wifi_ap_dhcp_end(struct sys_conf *cfg) {
  return cfg->wifi.ap.dhcp_end;
}
int         sys_conf_get_foo(struct sys_conf *cfg) {
  return cfg->foo;
}
const struct sys_conf_http *sys_conf_get_http(struct sys_conf *cfg) {
  return &cfg->http;
}
int         sys_conf_get_http_enable(struct sys_conf *cfg) {
  return cfg->http.enable;
}
int         sys_conf_get_http_port(struct sys_conf *cfg) {
  return cfg->http.port;
}
const struct sys_conf_debug *sys_conf_get_debug(struct sys_conf *cfg) {
  return &cfg->debug;
}
int         sys_conf_get_debug_level(struct sys_conf *cfg) {
  return cfg->debug.level;
}
const char *sys_conf_get_debug_dest(struct sys_conf *cfg) {
  return cfg->debug.dest;
}
double      sys_conf_get_debug_test_d1(struct sys_conf *cfg) {
  return cfg->debug.test_d1;
}
double      sys_conf_get_debug_test_d2(struct sys_conf *cfg) {
  return cfg->debug.test_d2;
}
/* }}} */

/* Setters {{{ */
void sys_conf_set_wifi_sta_ssid(struct sys_conf *cfg, const char *val) {
  mgos_conf_set_str(&cfg->wifi.sta.ssid, val);
}
void sys_conf_set_wifi_sta_pass(struct sys_conf *cfg, const char *val) {
  mgos_conf_set_str(&cfg->wifi.sta.pass, val);
}
void sys_conf_set_wifi_ap_ssid(struct sys_conf *cfg, const char *val) {
  mgos_conf_set_str(&cfg->wifi.ap.ssid, val);
}
void sys_conf_set_wifi_ap_pass(struct sys_conf *cfg, const char *val) {
  mgos_conf_set_str(&cfg->wifi.ap.pass, val);
}
void sys_conf_set_wifi_ap_channel(struct sys_conf *cfg, int         val) {
  cfg->wifi.ap.channel = val;
}
void sys_conf_set_wifi_ap_dhcp_end(struct sys_conf *cfg, const char *val) {
  mgos_conf_set_str(&cfg->wifi.ap.dhcp_end, val);
}
void sys_conf_set_foo(struct sys_conf *cfg, int         val) {
  cfg->foo = val;
}
void sys_conf_set_http_enable(struct sys_conf *cfg, int         val) {
  cfg->http.enable = val;
}
void sys_conf_set_http_port(struct sys_conf *cfg, int         val) {
  cfg->http.port = val;
}
void sys_conf_set_debug_level(struct sys_conf *cfg, int         val) {
  cfg->debug.level = val;
}
void sys_conf_set_debug_dest(struct sys_conf *cfg, const char *val) {
  mgos_conf_set_str(&cfg->debug.dest, val);
}
void sys_conf_set_debug_test_d1(struct sys_conf *cfg, double      val) {
  cfg->debug.test_d1 = val;
}
void sys_conf_set_debug_test_d2(struct sys_conf *cfg, double      val) {
  cfg->debug.test_d2 = val;
}
/* }}} */
