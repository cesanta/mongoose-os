/* clang-format off */
/*
 * Generated file - do not edit.
 * Command: ../../tools/mgos_gen_config.py --c_name=mgos_config --c_global_name=mgos_sys_config --dest_dir=./build data/sys_conf_wifi.yaml data/sys_conf_http.yaml data/sys_conf_debug.yaml data/sys_conf_overrides.yaml
 */

#include "mgos_config.h"

#include <stddef.h>

#include "mgos_config_util.h"

const struct mgos_conf_entry mgos_config_schema_[27] = {
  {.type = CONF_TYPE_OBJECT, .key = "", .offset = 0, .num_desc = 26},
  {.type = CONF_TYPE_OBJECT, .key = "wifi", .offset = offsetof(struct mgos_config, wifi), .num_desc = 8},
  {.type = CONF_TYPE_OBJECT, .key = "sta", .offset = offsetof(struct mgos_config, wifi.sta), .num_desc = 2},
  {.type = CONF_TYPE_STRING, .key = "ssid", .offset = offsetof(struct mgos_config, wifi.sta.ssid)},
  {.type = CONF_TYPE_STRING, .key = "pass", .offset = offsetof(struct mgos_config, wifi.sta.pass)},
  {.type = CONF_TYPE_OBJECT, .key = "ap", .offset = offsetof(struct mgos_config, wifi.ap), .num_desc = 4},
  {.type = CONF_TYPE_STRING, .key = "ssid", .offset = offsetof(struct mgos_config, wifi.ap.ssid)},
  {.type = CONF_TYPE_STRING, .key = "pass", .offset = offsetof(struct mgos_config, wifi.ap.pass)},
  {.type = CONF_TYPE_INT, .key = "channel", .offset = offsetof(struct mgos_config, wifi.ap.channel)},
  {.type = CONF_TYPE_STRING, .key = "dhcp_end", .offset = offsetof(struct mgos_config, wifi.ap.dhcp_end)},
  {.type = CONF_TYPE_INT, .key = "foo", .offset = offsetof(struct mgos_config, foo)},
  {.type = CONF_TYPE_OBJECT, .key = "http", .offset = offsetof(struct mgos_config, http), .num_desc = 2},
  {.type = CONF_TYPE_BOOL, .key = "enable", .offset = offsetof(struct mgos_config, http.enable)},
  {.type = CONF_TYPE_INT, .key = "port", .offset = offsetof(struct mgos_config, http.port)},
  {.type = CONF_TYPE_OBJECT, .key = "debug", .offset = offsetof(struct mgos_config, debug), .num_desc = 5},
  {.type = CONF_TYPE_INT, .key = "level", .offset = offsetof(struct mgos_config, debug.level)},
  {.type = CONF_TYPE_STRING, .key = "dest", .offset = offsetof(struct mgos_config, debug.dest)},
  {.type = CONF_TYPE_DOUBLE, .key = "test_d1", .offset = offsetof(struct mgos_config, debug.test_d1)},
  {.type = CONF_TYPE_DOUBLE, .key = "test_d2", .offset = offsetof(struct mgos_config, debug.test_d2)},
  {.type = CONF_TYPE_UNSIGNED_INT, .key = "test_ui", .offset = offsetof(struct mgos_config, debug.test_ui)},
  {.type = CONF_TYPE_OBJECT, .key = "test", .offset = offsetof(struct mgos_config, test), .num_desc = 6},
  {.type = CONF_TYPE_OBJECT, .key = "bar", .offset = offsetof(struct mgos_config, test.bar), .num_desc = 2},
  {.type = CONF_TYPE_BOOL, .key = "enable", .offset = offsetof(struct mgos_config, test.bar.enable)},
  {.type = CONF_TYPE_INT, .key = "param1", .offset = offsetof(struct mgos_config, test.bar.param1)},
  {.type = CONF_TYPE_OBJECT, .key = "bar1", .offset = offsetof(struct mgos_config, test.bar1), .num_desc = 2},
  {.type = CONF_TYPE_BOOL, .key = "enable", .offset = offsetof(struct mgos_config, test.bar1.enable)},
  {.type = CONF_TYPE_INT, .key = "param1", .offset = offsetof(struct mgos_config, test.bar1.param1)},
};

const struct mgos_conf_entry *mgos_config_schema() {
  return mgos_config_schema_;
}

/* Global instance */
struct mgos_config mgos_sys_config;
const struct mgos_config mgos_config_defaults = {
  .wifi.sta.ssid = NULL,
  .wifi.sta.pass = "so\nmany\nlines\n",
  .wifi.ap.ssid = "Quote \" me \\\\ please",
  .wifi.ap.pass = "\u043c\u0430\u043b\u043e\u0432\u0430\u0442\u043e \u0431\u0443\u0434\u0435\u0442",
  .wifi.ap.channel = 6,
  .wifi.ap.dhcp_end = "192.168.4.200",
  .foo = 123,
  .http.enable = 1,
  .http.port = 80,
  .debug.level = 2,
  .debug.dest = "uart1",
  .debug.test_d1 = 2.0,
  .debug.test_d2 = 0.0,
  .debug.test_ui = 4294967295,
  .test.bar.enable = 0,
  .test.bar.param1 = 111,
  .test.bar1.enable = 0,
  .test.bar1.param1 = 222,
};

/* wifi */
#define MGOS_CONFIG_HAVE_WIFI
#define MGOS_SYS_CONFIG_HAVE_WIFI
const struct mgos_config_wifi * mgos_config_get_wifi(struct mgos_config *cfg) {
  return &cfg->wifi;
}
const struct mgos_conf_entry *mgos_config_schema_wifi(void) {
  return mgos_conf_find_schema_entry("wifi", mgos_config_schema());
}
bool mgos_config_parse_wifi(struct mg_str json, struct mgos_config_wifi *cfg) {
  return mgos_conf_parse_sub(json, mgos_config_schema(), cfg);
}
bool mgos_config_copy_wifi(const struct mgos_config_wifi *src, struct mgos_config_wifi *dst) {
  return mgos_conf_copy(mgos_config_schema_wifi(), src, dst);
}
void mgos_config_free_wifi(struct mgos_config_wifi *cfg) {
  return mgos_conf_free(mgos_config_schema_wifi(), cfg);
}

/* wifi.sta */
#define MGOS_CONFIG_HAVE_WIFI_STA
#define MGOS_SYS_CONFIG_HAVE_WIFI_STA
const struct mgos_config_wifi_sta * mgos_config_get_wifi_sta(struct mgos_config *cfg) {
  return &cfg->wifi.sta;
}
const struct mgos_conf_entry *mgos_config_schema_wifi_sta(void) {
  return mgos_conf_find_schema_entry("wifi.sta", mgos_config_schema());
}
bool mgos_config_parse_wifi_sta(struct mg_str json, struct mgos_config_wifi_sta *cfg) {
  return mgos_conf_parse_sub(json, mgos_config_schema(), cfg);
}
bool mgos_config_copy_wifi_sta(const struct mgos_config_wifi_sta *src, struct mgos_config_wifi_sta *dst) {
  return mgos_conf_copy(mgos_config_schema_wifi_sta(), src, dst);
}
void mgos_config_free_wifi_sta(struct mgos_config_wifi_sta *cfg) {
  return mgos_conf_free(mgos_config_schema_wifi_sta(), cfg);
}

/* wifi.sta.ssid */
#define MGOS_CONFIG_HAVE_WIFI_STA_SSID
#define MGOS_SYS_CONFIG_HAVE_WIFI_STA_SSID
const char * mgos_config_get_wifi_sta_ssid(struct mgos_config *cfg) {
  return cfg->wifi.sta.ssid;
}
void mgos_config_set_wifi_sta_ssid(struct mgos_config *cfg, const char * v) {
  mgos_conf_set_str(&cfg->wifi.sta.ssid, v);
}

/* wifi.sta.pass */
#define MGOS_CONFIG_HAVE_WIFI_STA_PASS
#define MGOS_SYS_CONFIG_HAVE_WIFI_STA_PASS
const char * mgos_config_get_wifi_sta_pass(struct mgos_config *cfg) {
  return cfg->wifi.sta.pass;
}
void mgos_config_set_wifi_sta_pass(struct mgos_config *cfg, const char * v) {
  mgos_conf_set_str(&cfg->wifi.sta.pass, v);
}

/* wifi.ap */
#define MGOS_CONFIG_HAVE_WIFI_AP
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP
const struct mgos_config_wifi_ap * mgos_config_get_wifi_ap(struct mgos_config *cfg) {
  return &cfg->wifi.ap;
}
const struct mgos_conf_entry *mgos_config_schema_wifi_ap(void) {
  return mgos_conf_find_schema_entry("wifi.ap", mgos_config_schema());
}
bool mgos_config_parse_wifi_ap(struct mg_str json, struct mgos_config_wifi_ap *cfg) {
  return mgos_conf_parse_sub(json, mgos_config_schema(), cfg);
}
bool mgos_config_copy_wifi_ap(const struct mgos_config_wifi_ap *src, struct mgos_config_wifi_ap *dst) {
  return mgos_conf_copy(mgos_config_schema_wifi_ap(), src, dst);
}
void mgos_config_free_wifi_ap(struct mgos_config_wifi_ap *cfg) {
  return mgos_conf_free(mgos_config_schema_wifi_ap(), cfg);
}

/* wifi.ap.ssid */
#define MGOS_CONFIG_HAVE_WIFI_AP_SSID
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_SSID
const char * mgos_config_get_wifi_ap_ssid(struct mgos_config *cfg) {
  return cfg->wifi.ap.ssid;
}
void mgos_config_set_wifi_ap_ssid(struct mgos_config *cfg, const char * v) {
  mgos_conf_set_str(&cfg->wifi.ap.ssid, v);
}

/* wifi.ap.pass */
#define MGOS_CONFIG_HAVE_WIFI_AP_PASS
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_PASS
const char * mgos_config_get_wifi_ap_pass(struct mgos_config *cfg) {
  return cfg->wifi.ap.pass;
}
void mgos_config_set_wifi_ap_pass(struct mgos_config *cfg, const char * v) {
  mgos_conf_set_str(&cfg->wifi.ap.pass, v);
}

/* wifi.ap.channel */
#define MGOS_CONFIG_HAVE_WIFI_AP_CHANNEL
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_CHANNEL
int mgos_config_get_wifi_ap_channel(struct mgos_config *cfg) {
  return cfg->wifi.ap.channel;
}
void mgos_config_set_wifi_ap_channel(struct mgos_config *cfg, int v) {
  cfg->wifi.ap.channel = v;
}

/* wifi.ap.dhcp_end */
#define MGOS_CONFIG_HAVE_WIFI_AP_DHCP_END
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_DHCP_END
const char * mgos_config_get_wifi_ap_dhcp_end(struct mgos_config *cfg) {
  return cfg->wifi.ap.dhcp_end;
}
void mgos_config_set_wifi_ap_dhcp_end(struct mgos_config *cfg, const char * v) {
  mgos_conf_set_str(&cfg->wifi.ap.dhcp_end, v);
}

/* foo */
#define MGOS_CONFIG_HAVE_FOO
#define MGOS_SYS_CONFIG_HAVE_FOO
int mgos_config_get_foo(struct mgos_config *cfg) {
  return cfg->foo;
}
void mgos_config_set_foo(struct mgos_config *cfg, int v) {
  cfg->foo = v;
}

/* http */
#define MGOS_CONFIG_HAVE_HTTP
#define MGOS_SYS_CONFIG_HAVE_HTTP
const struct mgos_config_http * mgos_config_get_http(struct mgos_config *cfg) {
  return &cfg->http;
}
const struct mgos_conf_entry *mgos_config_schema_http(void) {
  return mgos_conf_find_schema_entry("http", mgos_config_schema());
}
bool mgos_config_parse_http(struct mg_str json, struct mgos_config_http *cfg) {
  return mgos_conf_parse_sub(json, mgos_config_schema(), cfg);
}
bool mgos_config_copy_http(const struct mgos_config_http *src, struct mgos_config_http *dst) {
  return mgos_conf_copy(mgos_config_schema_http(), src, dst);
}
void mgos_config_free_http(struct mgos_config_http *cfg) {
  return mgos_conf_free(mgos_config_schema_http(), cfg);
}

/* http.enable */
#define MGOS_CONFIG_HAVE_HTTP_ENABLE
#define MGOS_SYS_CONFIG_HAVE_HTTP_ENABLE
int mgos_config_get_http_enable(struct mgos_config *cfg) {
  return cfg->http.enable;
}
void mgos_config_set_http_enable(struct mgos_config *cfg, int v) {
  cfg->http.enable = v;
}

/* http.port */
#define MGOS_CONFIG_HAVE_HTTP_PORT
#define MGOS_SYS_CONFIG_HAVE_HTTP_PORT
int mgos_config_get_http_port(struct mgos_config *cfg) {
  return cfg->http.port;
}
void mgos_config_set_http_port(struct mgos_config *cfg, int v) {
  cfg->http.port = v;
}

/* debug */
#define MGOS_CONFIG_HAVE_DEBUG
#define MGOS_SYS_CONFIG_HAVE_DEBUG
const struct mgos_config_debug * mgos_config_get_debug(struct mgos_config *cfg) {
  return &cfg->debug;
}
const struct mgos_conf_entry *mgos_config_schema_debug(void) {
  return mgos_conf_find_schema_entry("debug", mgos_config_schema());
}
bool mgos_config_parse_debug(struct mg_str json, struct mgos_config_debug *cfg) {
  return mgos_conf_parse_sub(json, mgos_config_schema(), cfg);
}
bool mgos_config_copy_debug(const struct mgos_config_debug *src, struct mgos_config_debug *dst) {
  return mgos_conf_copy(mgos_config_schema_debug(), src, dst);
}
void mgos_config_free_debug(struct mgos_config_debug *cfg) {
  return mgos_conf_free(mgos_config_schema_debug(), cfg);
}

/* debug.level */
#define MGOS_CONFIG_HAVE_DEBUG_LEVEL
#define MGOS_SYS_CONFIG_HAVE_DEBUG_LEVEL
int mgos_config_get_debug_level(struct mgos_config *cfg) {
  return cfg->debug.level;
}
void mgos_config_set_debug_level(struct mgos_config *cfg, int v) {
  cfg->debug.level = v;
}

/* debug.dest */
#define MGOS_CONFIG_HAVE_DEBUG_DEST
#define MGOS_SYS_CONFIG_HAVE_DEBUG_DEST
const char * mgos_config_get_debug_dest(struct mgos_config *cfg) {
  return cfg->debug.dest;
}
void mgos_config_set_debug_dest(struct mgos_config *cfg, const char * v) {
  mgos_conf_set_str(&cfg->debug.dest, v);
}

/* debug.test_d1 */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_D1
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_D1
double mgos_config_get_debug_test_d1(struct mgos_config *cfg) {
  return cfg->debug.test_d1;
}
void mgos_config_set_debug_test_d1(struct mgos_config *cfg, double v) {
  cfg->debug.test_d1 = v;
}

/* debug.test_d2 */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_D2
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_D2
double mgos_config_get_debug_test_d2(struct mgos_config *cfg) {
  return cfg->debug.test_d2;
}
void mgos_config_set_debug_test_d2(struct mgos_config *cfg, double v) {
  cfg->debug.test_d2 = v;
}

/* debug.test_ui */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_UI
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_UI
unsigned int mgos_config_get_debug_test_ui(struct mgos_config *cfg) {
  return cfg->debug.test_ui;
}
void mgos_config_set_debug_test_ui(struct mgos_config *cfg, unsigned int v) {
  cfg->debug.test_ui = v;
}

/* test */
#define MGOS_CONFIG_HAVE_TEST
#define MGOS_SYS_CONFIG_HAVE_TEST
const struct mgos_config_test * mgos_config_get_test(struct mgos_config *cfg) {
  return &cfg->test;
}
const struct mgos_conf_entry *mgos_config_schema_test(void) {
  return mgos_conf_find_schema_entry("test", mgos_config_schema());
}
bool mgos_config_parse_test(struct mg_str json, struct mgos_config_test *cfg) {
  return mgos_conf_parse_sub(json, mgos_config_schema(), cfg);
}
bool mgos_config_copy_test(const struct mgos_config_test *src, struct mgos_config_test *dst) {
  return mgos_conf_copy(mgos_config_schema_test(), src, dst);
}
void mgos_config_free_test(struct mgos_config_test *cfg) {
  return mgos_conf_free(mgos_config_schema_test(), cfg);
}

/* test.bar */
#define MGOS_CONFIG_HAVE_TEST_BAR
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR
const struct mgos_config_test_bar * mgos_config_get_test_bar(struct mgos_config *cfg) {
  return &cfg->test.bar;
}
const struct mgos_conf_entry *mgos_config_schema_test_bar(void) {
  return mgos_conf_find_schema_entry("test.bar", mgos_config_schema());
}
bool mgos_config_parse_test_bar(struct mg_str json, struct mgos_config_test_bar *cfg) {
  return mgos_conf_parse_sub(json, mgos_config_schema(), cfg);
}
bool mgos_config_copy_test_bar(const struct mgos_config_test_bar *src, struct mgos_config_test_bar *dst) {
  return mgos_conf_copy(mgos_config_schema_test_bar(), src, dst);
}
void mgos_config_free_test_bar(struct mgos_config_test_bar *cfg) {
  return mgos_conf_free(mgos_config_schema_test_bar(), cfg);
}

/* test.bar.enable */
#define MGOS_CONFIG_HAVE_TEST_BAR_ENABLE
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR_ENABLE
int mgos_config_get_test_bar_enable(struct mgos_config *cfg) {
  return cfg->test.bar.enable;
}
void mgos_config_set_test_bar_enable(struct mgos_config *cfg, int v) {
  cfg->test.bar.enable = v;
}

/* test.bar.param1 */
#define MGOS_CONFIG_HAVE_TEST_BAR_PARAM1
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR_PARAM1
int mgos_config_get_test_bar_param1(struct mgos_config *cfg) {
  return cfg->test.bar.param1;
}
void mgos_config_set_test_bar_param1(struct mgos_config *cfg, int v) {
  cfg->test.bar.param1 = v;
}

/* test.bar1 */
#define MGOS_CONFIG_HAVE_TEST_BAR1
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1
const struct mgos_config_test_bar * mgos_config_get_test_bar1(struct mgos_config *cfg) {
  return &cfg->test.bar1;
}

/* test.bar1.enable */
#define MGOS_CONFIG_HAVE_TEST_BAR1_ENABLE
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1_ENABLE
int mgos_config_get_test_bar1_enable(struct mgos_config *cfg) {
  return cfg->test.bar1.enable;
}
void mgos_config_set_test_bar1_enable(struct mgos_config *cfg, int v) {
  cfg->test.bar1.enable = v;
}

/* test.bar1.param1 */
#define MGOS_CONFIG_HAVE_TEST_BAR1_PARAM1
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1_PARAM1
int mgos_config_get_test_bar1_param1(struct mgos_config *cfg) {
  return cfg->test.bar1.param1;
}
void mgos_config_set_test_bar1_param1(struct mgos_config *cfg, int v) {
  cfg->test.bar1.param1 = v;
}
bool mgos_sys_config_get(const struct mg_str key, struct mg_str *value) {
  return mgos_config_get(key, value, &mgos_sys_config, mgos_config_schema());
}
bool mgos_sys_config_set(const struct mg_str key, const struct mg_str value, bool free_strings) {
  return mgos_config_set(key, value, &mgos_sys_config, mgos_config_schema(), free_strings);
}
