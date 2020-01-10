/* clang-format off */
/*
 * Generated file - do not edit.
 * Command: ../../tools/mgos_gen_config.py --c_name=mgos_config --c_global_name=mgos_sys_config --dest_dir=./build data/sys_conf_wifi.yaml data/sys_conf_http.yaml data/sys_conf_debug.yaml data/sys_conf_overrides.yaml
 */

#pragma once

#include <stdbool.h>
#include "common/mg_str.h"

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_config_wifi_sta {
  const char * ssid;
  const char * pass;
};

struct mgos_config_wifi_ap {
  const char * ssid;
  const char * pass;
  int channel;
  const char * dhcp_end;
};

struct mgos_config_wifi {
  struct mgos_config_wifi_sta sta;
  struct mgos_config_wifi_ap ap;
};

struct mgos_config_http {
  int enable;
  int port;
};

struct mgos_config_debug {
  int level;
  const char * dest;
  double test_d1;
  double test_d2;
  unsigned int test_ui;
};

struct mgos_config_test_bar {
  int enable;
  int param1;
};

struct mgos_config_test {
  struct mgos_config_test_bar bar;
  struct mgos_config_test_bar bar1;
};

struct mgos_config {
  struct mgos_config_wifi wifi;
  int foo;
  struct mgos_config_http http;
  struct mgos_config_debug debug;
  struct mgos_config_test test;
};


const struct mgos_conf_entry *mgos_config_schema();

extern struct mgos_config mgos_sys_config;
extern const struct mgos_config mgos_config_defaults;

/* wifi */
#define MGOS_CONFIG_HAVE_WIFI
#define MGOS_SYS_CONFIG_HAVE_WIFI
const struct mgos_config_wifi * mgos_config_get_wifi(struct mgos_config *cfg);
static inline const struct mgos_config_wifi * mgos_sys_config_get_wifi(void) { return mgos_config_get_wifi(&mgos_sys_config); }

/* wifi.sta */
#define MGOS_CONFIG_HAVE_WIFI_STA
#define MGOS_SYS_CONFIG_HAVE_WIFI_STA
const struct mgos_config_wifi_sta * mgos_config_get_wifi_sta(struct mgos_config *cfg);
static inline const struct mgos_config_wifi_sta * mgos_sys_config_get_wifi_sta(void) { return mgos_config_get_wifi_sta(&mgos_sys_config); }

/* wifi.sta.ssid */
#define MGOS_CONFIG_HAVE_WIFI_STA_SSID
#define MGOS_SYS_CONFIG_HAVE_WIFI_STA_SSID
const char * mgos_config_get_wifi_sta_ssid(struct mgos_config *cfg);
static inline const char * mgos_sys_config_get_wifi_sta_ssid(void) { return mgos_config_get_wifi_sta_ssid(&mgos_sys_config); }
void mgos_config_set_wifi_sta_ssid(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_wifi_sta_ssid(const char * v) { mgos_config_set_wifi_sta_ssid(&mgos_sys_config, v); }

/* wifi.sta.pass */
#define MGOS_CONFIG_HAVE_WIFI_STA_PASS
#define MGOS_SYS_CONFIG_HAVE_WIFI_STA_PASS
const char * mgos_config_get_wifi_sta_pass(struct mgos_config *cfg);
static inline const char * mgos_sys_config_get_wifi_sta_pass(void) { return mgos_config_get_wifi_sta_pass(&mgos_sys_config); }
void mgos_config_set_wifi_sta_pass(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_wifi_sta_pass(const char * v) { mgos_config_set_wifi_sta_pass(&mgos_sys_config, v); }

/* wifi.ap */
#define MGOS_CONFIG_HAVE_WIFI_AP
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP
const struct mgos_config_wifi_ap * mgos_config_get_wifi_ap(struct mgos_config *cfg);
static inline const struct mgos_config_wifi_ap * mgos_sys_config_get_wifi_ap(void) { return mgos_config_get_wifi_ap(&mgos_sys_config); }

/* wifi.ap.ssid */
#define MGOS_CONFIG_HAVE_WIFI_AP_SSID
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_SSID
const char * mgos_config_get_wifi_ap_ssid(struct mgos_config *cfg);
static inline const char * mgos_sys_config_get_wifi_ap_ssid(void) { return mgos_config_get_wifi_ap_ssid(&mgos_sys_config); }
void mgos_config_set_wifi_ap_ssid(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_wifi_ap_ssid(const char * v) { mgos_config_set_wifi_ap_ssid(&mgos_sys_config, v); }

/* wifi.ap.pass */
#define MGOS_CONFIG_HAVE_WIFI_AP_PASS
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_PASS
const char * mgos_config_get_wifi_ap_pass(struct mgos_config *cfg);
static inline const char * mgos_sys_config_get_wifi_ap_pass(void) { return mgos_config_get_wifi_ap_pass(&mgos_sys_config); }
void mgos_config_set_wifi_ap_pass(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_wifi_ap_pass(const char * v) { mgos_config_set_wifi_ap_pass(&mgos_sys_config, v); }

/* wifi.ap.channel */
#define MGOS_CONFIG_HAVE_WIFI_AP_CHANNEL
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_CHANNEL
int mgos_config_get_wifi_ap_channel(struct mgos_config *cfg);
static inline int mgos_sys_config_get_wifi_ap_channel(void) { return mgos_config_get_wifi_ap_channel(&mgos_sys_config); }
void mgos_config_set_wifi_ap_channel(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_wifi_ap_channel(int v) { mgos_config_set_wifi_ap_channel(&mgos_sys_config, v); }

/* wifi.ap.dhcp_end */
#define MGOS_CONFIG_HAVE_WIFI_AP_DHCP_END
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_DHCP_END
const char * mgos_config_get_wifi_ap_dhcp_end(struct mgos_config *cfg);
static inline const char * mgos_sys_config_get_wifi_ap_dhcp_end(void) { return mgos_config_get_wifi_ap_dhcp_end(&mgos_sys_config); }
void mgos_config_set_wifi_ap_dhcp_end(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_wifi_ap_dhcp_end(const char * v) { mgos_config_set_wifi_ap_dhcp_end(&mgos_sys_config, v); }

/* foo */
#define MGOS_CONFIG_HAVE_FOO
#define MGOS_SYS_CONFIG_HAVE_FOO
int mgos_config_get_foo(struct mgos_config *cfg);
static inline int mgos_sys_config_get_foo(void) { return mgos_config_get_foo(&mgos_sys_config); }
void mgos_config_set_foo(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_foo(int v) { mgos_config_set_foo(&mgos_sys_config, v); }

/* http */
#define MGOS_CONFIG_HAVE_HTTP
#define MGOS_SYS_CONFIG_HAVE_HTTP
const struct mgos_config_http * mgos_config_get_http(struct mgos_config *cfg);
static inline const struct mgos_config_http * mgos_sys_config_get_http(void) { return mgos_config_get_http(&mgos_sys_config); }

/* http.enable */
#define MGOS_CONFIG_HAVE_HTTP_ENABLE
#define MGOS_SYS_CONFIG_HAVE_HTTP_ENABLE
int mgos_config_get_http_enable(struct mgos_config *cfg);
static inline int mgos_sys_config_get_http_enable(void) { return mgos_config_get_http_enable(&mgos_sys_config); }
void mgos_config_set_http_enable(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_http_enable(int v) { mgos_config_set_http_enable(&mgos_sys_config, v); }

/* http.port */
#define MGOS_CONFIG_HAVE_HTTP_PORT
#define MGOS_SYS_CONFIG_HAVE_HTTP_PORT
int mgos_config_get_http_port(struct mgos_config *cfg);
static inline int mgos_sys_config_get_http_port(void) { return mgos_config_get_http_port(&mgos_sys_config); }
void mgos_config_set_http_port(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_http_port(int v) { mgos_config_set_http_port(&mgos_sys_config, v); }

/* debug */
#define MGOS_CONFIG_HAVE_DEBUG
#define MGOS_SYS_CONFIG_HAVE_DEBUG
const struct mgos_config_debug * mgos_config_get_debug(struct mgos_config *cfg);
static inline const struct mgos_config_debug * mgos_sys_config_get_debug(void) { return mgos_config_get_debug(&mgos_sys_config); }

/* debug.level */
#define MGOS_CONFIG_HAVE_DEBUG_LEVEL
#define MGOS_SYS_CONFIG_HAVE_DEBUG_LEVEL
int mgos_config_get_debug_level(struct mgos_config *cfg);
static inline int mgos_sys_config_get_debug_level(void) { return mgos_config_get_debug_level(&mgos_sys_config); }
void mgos_config_set_debug_level(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_debug_level(int v) { mgos_config_set_debug_level(&mgos_sys_config, v); }

/* debug.dest */
#define MGOS_CONFIG_HAVE_DEBUG_DEST
#define MGOS_SYS_CONFIG_HAVE_DEBUG_DEST
const char * mgos_config_get_debug_dest(struct mgos_config *cfg);
static inline const char * mgos_sys_config_get_debug_dest(void) { return mgos_config_get_debug_dest(&mgos_sys_config); }
void mgos_config_set_debug_dest(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_debug_dest(const char * v) { mgos_config_set_debug_dest(&mgos_sys_config, v); }

/* debug.test_d1 */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_D1
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_D1
double mgos_config_get_debug_test_d1(struct mgos_config *cfg);
static inline double mgos_sys_config_get_debug_test_d1(void) { return mgos_config_get_debug_test_d1(&mgos_sys_config); }
void mgos_config_set_debug_test_d1(struct mgos_config *cfg, double v);
static inline void mgos_sys_config_set_debug_test_d1(double v) { mgos_config_set_debug_test_d1(&mgos_sys_config, v); }

/* debug.test_d2 */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_D2
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_D2
double mgos_config_get_debug_test_d2(struct mgos_config *cfg);
static inline double mgos_sys_config_get_debug_test_d2(void) { return mgos_config_get_debug_test_d2(&mgos_sys_config); }
void mgos_config_set_debug_test_d2(struct mgos_config *cfg, double v);
static inline void mgos_sys_config_set_debug_test_d2(double v) { mgos_config_set_debug_test_d2(&mgos_sys_config, v); }

/* debug.test_ui */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_UI
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_UI
unsigned int mgos_config_get_debug_test_ui(struct mgos_config *cfg);
static inline unsigned int mgos_sys_config_get_debug_test_ui(void) { return mgos_config_get_debug_test_ui(&mgos_sys_config); }
void mgos_config_set_debug_test_ui(struct mgos_config *cfg, unsigned int v);
static inline void mgos_sys_config_set_debug_test_ui(unsigned int v) { mgos_config_set_debug_test_ui(&mgos_sys_config, v); }

/* test */
#define MGOS_CONFIG_HAVE_TEST
#define MGOS_SYS_CONFIG_HAVE_TEST
const struct mgos_config_test * mgos_config_get_test(struct mgos_config *cfg);
static inline const struct mgos_config_test * mgos_sys_config_get_test(void) { return mgos_config_get_test(&mgos_sys_config); }

/* test.bar */
#define MGOS_CONFIG_HAVE_TEST_BAR
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR
const struct mgos_config_test_bar * mgos_config_get_test_bar(struct mgos_config *cfg);
static inline const struct mgos_config_test_bar * mgos_sys_config_get_test_bar(void) { return mgos_config_get_test_bar(&mgos_sys_config); }

/* test.bar.enable */
#define MGOS_CONFIG_HAVE_TEST_BAR_ENABLE
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR_ENABLE
int mgos_config_get_test_bar_enable(struct mgos_config *cfg);
static inline int mgos_sys_config_get_test_bar_enable(void) { return mgos_config_get_test_bar_enable(&mgos_sys_config); }
void mgos_config_set_test_bar_enable(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_test_bar_enable(int v) { mgos_config_set_test_bar_enable(&mgos_sys_config, v); }

/* test.bar.param1 */
#define MGOS_CONFIG_HAVE_TEST_BAR_PARAM1
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR_PARAM1
int mgos_config_get_test_bar_param1(struct mgos_config *cfg);
static inline int mgos_sys_config_get_test_bar_param1(void) { return mgos_config_get_test_bar_param1(&mgos_sys_config); }
void mgos_config_set_test_bar_param1(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_test_bar_param1(int v) { mgos_config_set_test_bar_param1(&mgos_sys_config, v); }

/* test.bar1 */
#define MGOS_CONFIG_HAVE_TEST_BAR1
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1
const struct mgos_config_test_bar * mgos_config_get_test_bar1(struct mgos_config *cfg);
static inline const struct mgos_config_test_bar * mgos_sys_config_get_test_bar1(void) { return mgos_config_get_test_bar1(&mgos_sys_config); }

/* test.bar1.enable */
#define MGOS_CONFIG_HAVE_TEST_BAR1_ENABLE
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1_ENABLE
int mgos_config_get_test_bar1_enable(struct mgos_config *cfg);
static inline int mgos_sys_config_get_test_bar1_enable(void) { return mgos_config_get_test_bar1_enable(&mgos_sys_config); }
void mgos_config_set_test_bar1_enable(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_test_bar1_enable(int v) { mgos_config_set_test_bar1_enable(&mgos_sys_config, v); }

/* test.bar1.param1 */
#define MGOS_CONFIG_HAVE_TEST_BAR1_PARAM1
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1_PARAM1
int mgos_config_get_test_bar1_param1(struct mgos_config *cfg);
static inline int mgos_sys_config_get_test_bar1_param1(void) { return mgos_config_get_test_bar1_param1(&mgos_sys_config); }
void mgos_config_set_test_bar1_param1(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_test_bar1_param1(int v) { mgos_config_set_test_bar1_param1(&mgos_sys_config, v); }

bool mgos_sys_config_get(const struct mg_str key, struct mg_str *value);
bool mgos_sys_config_set(const struct mg_str key, const struct mg_str value, bool free_strings);

#ifdef __cplusplus
}
#endif
