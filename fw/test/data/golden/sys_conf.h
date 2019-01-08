/* clang-format off */
/*
 * Generated file - do not edit.
 * Command: ../../fw/tools/gen_sys_config.py --c_name=sys_conf --c_global_name=sys_conf_global --dest_dir=.build data/sys_conf_wifi.yaml data/sys_conf_http.yaml data/sys_conf_debug.yaml data/sys_conf_overrides.yaml
 */

#pragma once

#include "mgos_config_util.h"

#ifdef __cplusplus
extern "C" {
#endif

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
  double test_d1;
  double test_d2;
};

struct sys_conf_test_bar {
  int enable;
  int param1;
};

struct sys_conf_test {
  struct sys_conf_test_bar bar;
  struct sys_conf_test_bar bar1;
};

struct sys_conf {
  struct sys_conf_wifi wifi;
  int foo;
  struct sys_conf_http http;
  struct sys_conf_debug debug;
  struct sys_conf_test test;
};


const struct mgos_conf_entry *sys_conf_schema();

/* Parametrized accessor prototypes {{{ */
#define SYS_CONF_HAVE_WIFI
const struct sys_conf_wifi *sys_conf_get_wifi(struct sys_conf *cfg);
#define SYS_CONF_HAVE_WIFI_STA
const struct sys_conf_wifi_sta *sys_conf_get_wifi_sta(struct sys_conf *cfg);
#define SYS_CONF_HAVE_WIFI_STA_SSID
const char *sys_conf_get_wifi_sta_ssid(struct sys_conf *cfg);
#define SYS_CONF_HAVE_WIFI_STA_PASS
const char *sys_conf_get_wifi_sta_pass(struct sys_conf *cfg);
#define SYS_CONF_HAVE_WIFI_AP
const struct sys_conf_wifi_ap *sys_conf_get_wifi_ap(struct sys_conf *cfg);
#define SYS_CONF_HAVE_WIFI_AP_SSID
const char *sys_conf_get_wifi_ap_ssid(struct sys_conf *cfg);
#define SYS_CONF_HAVE_WIFI_AP_PASS
const char *sys_conf_get_wifi_ap_pass(struct sys_conf *cfg);
#define SYS_CONF_HAVE_WIFI_AP_CHANNEL
int         sys_conf_get_wifi_ap_channel(struct sys_conf *cfg);
#define SYS_CONF_HAVE_WIFI_AP_DHCP_END
const char *sys_conf_get_wifi_ap_dhcp_end(struct sys_conf *cfg);
#define SYS_CONF_HAVE_FOO
int         sys_conf_get_foo(struct sys_conf *cfg);
#define SYS_CONF_HAVE_HTTP
const struct sys_conf_http *sys_conf_get_http(struct sys_conf *cfg);
#define SYS_CONF_HAVE_HTTP_ENABLE
int         sys_conf_get_http_enable(struct sys_conf *cfg);
#define SYS_CONF_HAVE_HTTP_PORT
int         sys_conf_get_http_port(struct sys_conf *cfg);
#define SYS_CONF_HAVE_DEBUG
const struct sys_conf_debug *sys_conf_get_debug(struct sys_conf *cfg);
#define SYS_CONF_HAVE_DEBUG_LEVEL
int         sys_conf_get_debug_level(struct sys_conf *cfg);
#define SYS_CONF_HAVE_DEBUG_DEST
const char *sys_conf_get_debug_dest(struct sys_conf *cfg);
#define SYS_CONF_HAVE_DEBUG_TEST_D1
double      sys_conf_get_debug_test_d1(struct sys_conf *cfg);
#define SYS_CONF_HAVE_DEBUG_TEST_D2
double      sys_conf_get_debug_test_d2(struct sys_conf *cfg);
#define SYS_CONF_HAVE_TEST
const struct sys_conf_test *sys_conf_get_test(struct sys_conf *cfg);
#define SYS_CONF_HAVE_TEST_BAR
const struct sys_conf_test_bar *sys_conf_get_test_bar(struct sys_conf *cfg);
#define SYS_CONF_HAVE_TEST_BAR_ENABLE
int         sys_conf_get_test_bar_enable(struct sys_conf *cfg);
#define SYS_CONF_HAVE_TEST_BAR_PARAM1
int         sys_conf_get_test_bar_param1(struct sys_conf *cfg);
#define SYS_CONF_HAVE_TEST_BAR1
const struct sys_conf_test_bar *sys_conf_get_test_bar1(struct sys_conf *cfg);
#define SYS_CONF_HAVE_TEST_BAR1_ENABLE
int         sys_conf_get_test_bar1_enable(struct sys_conf *cfg);
#define SYS_CONF_HAVE_TEST_BAR1_PARAM1
int         sys_conf_get_test_bar1_param1(struct sys_conf *cfg);

void sys_conf_set_wifi_sta_ssid(struct sys_conf *cfg, const char *val);
void sys_conf_set_wifi_sta_pass(struct sys_conf *cfg, const char *val);
void sys_conf_set_wifi_ap_ssid(struct sys_conf *cfg, const char *val);
void sys_conf_set_wifi_ap_pass(struct sys_conf *cfg, const char *val);
void sys_conf_set_wifi_ap_channel(struct sys_conf *cfg, int         val);
void sys_conf_set_wifi_ap_dhcp_end(struct sys_conf *cfg, const char *val);
void sys_conf_set_foo(struct sys_conf *cfg, int         val);
void sys_conf_set_http_enable(struct sys_conf *cfg, int         val);
void sys_conf_set_http_port(struct sys_conf *cfg, int         val);
void sys_conf_set_debug_level(struct sys_conf *cfg, int         val);
void sys_conf_set_debug_dest(struct sys_conf *cfg, const char *val);
void sys_conf_set_debug_test_d1(struct sys_conf *cfg, double      val);
void sys_conf_set_debug_test_d2(struct sys_conf *cfg, double      val);
void sys_conf_set_test_bar_enable(struct sys_conf *cfg, int         val);
void sys_conf_set_test_bar_param1(struct sys_conf *cfg, int         val);
void sys_conf_set_test_bar1_enable(struct sys_conf *cfg, int         val);
void sys_conf_set_test_bar1_param1(struct sys_conf *cfg, int         val);
/* }}} */

extern struct sys_conf sys_conf_global;

static inline bool sys_conf_global_get(const struct mg_str key, struct mg_str *value) { return mgos_config_get(key, value, &sys_conf_global, sys_conf_schema()); }
static inline bool sys_conf_global_set(const struct mg_str key, const struct mg_str value, bool free_strings) { return mgos_config_set(key, value, &sys_conf_global, sys_conf_schema(), free_strings); }

#define SYS_CONF_GLOBAL_HAVE_WIFI
static inline const struct sys_conf_wifi *sys_conf_global_get_wifi(void) { return sys_conf_get_wifi(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_WIFI_STA
static inline const struct sys_conf_wifi_sta *sys_conf_global_get_wifi_sta(void) { return sys_conf_get_wifi_sta(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_WIFI_STA_SSID
static inline const char *sys_conf_global_get_wifi_sta_ssid(void) { return sys_conf_get_wifi_sta_ssid(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_WIFI_STA_PASS
static inline const char *sys_conf_global_get_wifi_sta_pass(void) { return sys_conf_get_wifi_sta_pass(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_WIFI_AP
static inline const struct sys_conf_wifi_ap *sys_conf_global_get_wifi_ap(void) { return sys_conf_get_wifi_ap(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_WIFI_AP_SSID
static inline const char *sys_conf_global_get_wifi_ap_ssid(void) { return sys_conf_get_wifi_ap_ssid(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_WIFI_AP_PASS
static inline const char *sys_conf_global_get_wifi_ap_pass(void) { return sys_conf_get_wifi_ap_pass(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_WIFI_AP_CHANNEL
static inline int         sys_conf_global_get_wifi_ap_channel(void) { return sys_conf_get_wifi_ap_channel(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_WIFI_AP_DHCP_END
static inline const char *sys_conf_global_get_wifi_ap_dhcp_end(void) { return sys_conf_get_wifi_ap_dhcp_end(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_FOO
static inline int         sys_conf_global_get_foo(void) { return sys_conf_get_foo(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_HTTP
static inline const struct sys_conf_http *sys_conf_global_get_http(void) { return sys_conf_get_http(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_HTTP_ENABLE
static inline int         sys_conf_global_get_http_enable(void) { return sys_conf_get_http_enable(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_HTTP_PORT
static inline int         sys_conf_global_get_http_port(void) { return sys_conf_get_http_port(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_DEBUG
static inline const struct sys_conf_debug *sys_conf_global_get_debug(void) { return sys_conf_get_debug(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_DEBUG_LEVEL
static inline int         sys_conf_global_get_debug_level(void) { return sys_conf_get_debug_level(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_DEBUG_DEST
static inline const char *sys_conf_global_get_debug_dest(void) { return sys_conf_get_debug_dest(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_DEBUG_TEST_D1
static inline double      sys_conf_global_get_debug_test_d1(void) { return sys_conf_get_debug_test_d1(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_DEBUG_TEST_D2
static inline double      sys_conf_global_get_debug_test_d2(void) { return sys_conf_get_debug_test_d2(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_TEST
static inline const struct sys_conf_test *sys_conf_global_get_test(void) { return sys_conf_get_test(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_TEST_BAR
static inline const struct sys_conf_test_bar *sys_conf_global_get_test_bar(void) { return sys_conf_get_test_bar(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_TEST_BAR_ENABLE
static inline int         sys_conf_global_get_test_bar_enable(void) { return sys_conf_get_test_bar_enable(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_TEST_BAR_PARAM1
static inline int         sys_conf_global_get_test_bar_param1(void) { return sys_conf_get_test_bar_param1(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_TEST_BAR1
static inline const struct sys_conf_test_bar *sys_conf_global_get_test_bar1(void) { return sys_conf_get_test_bar1(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_TEST_BAR1_ENABLE
static inline int         sys_conf_global_get_test_bar1_enable(void) { return sys_conf_get_test_bar1_enable(&sys_conf_global); }
#define SYS_CONF_GLOBAL_HAVE_TEST_BAR1_PARAM1
static inline int         sys_conf_global_get_test_bar1_param1(void) { return sys_conf_get_test_bar1_param1(&sys_conf_global); }

static inline void sys_conf_global_set_wifi_sta_ssid(const char *val) { sys_conf_set_wifi_sta_ssid(&sys_conf_global, val); }
static inline void sys_conf_global_set_wifi_sta_pass(const char *val) { sys_conf_set_wifi_sta_pass(&sys_conf_global, val); }
static inline void sys_conf_global_set_wifi_ap_ssid(const char *val) { sys_conf_set_wifi_ap_ssid(&sys_conf_global, val); }
static inline void sys_conf_global_set_wifi_ap_pass(const char *val) { sys_conf_set_wifi_ap_pass(&sys_conf_global, val); }
static inline void sys_conf_global_set_wifi_ap_channel(int         val) { sys_conf_set_wifi_ap_channel(&sys_conf_global, val); }
static inline void sys_conf_global_set_wifi_ap_dhcp_end(const char *val) { sys_conf_set_wifi_ap_dhcp_end(&sys_conf_global, val); }
static inline void sys_conf_global_set_foo(int         val) { sys_conf_set_foo(&sys_conf_global, val); }
static inline void sys_conf_global_set_http_enable(int         val) { sys_conf_set_http_enable(&sys_conf_global, val); }
static inline void sys_conf_global_set_http_port(int         val) { sys_conf_set_http_port(&sys_conf_global, val); }
static inline void sys_conf_global_set_debug_level(int         val) { sys_conf_set_debug_level(&sys_conf_global, val); }
static inline void sys_conf_global_set_debug_dest(const char *val) { sys_conf_set_debug_dest(&sys_conf_global, val); }
static inline void sys_conf_global_set_debug_test_d1(double      val) { sys_conf_set_debug_test_d1(&sys_conf_global, val); }
static inline void sys_conf_global_set_debug_test_d2(double      val) { sys_conf_set_debug_test_d2(&sys_conf_global, val); }
static inline void sys_conf_global_set_test_bar_enable(int         val) { sys_conf_set_test_bar_enable(&sys_conf_global, val); }
static inline void sys_conf_global_set_test_bar_param1(int         val) { sys_conf_set_test_bar_param1(&sys_conf_global, val); }
static inline void sys_conf_global_set_test_bar1_enable(int         val) { sys_conf_set_test_bar1_enable(&sys_conf_global, val); }
static inline void sys_conf_global_set_test_bar1_param1(int         val) { sys_conf_set_test_bar1_param1(&sys_conf_global, val); }

#ifdef __cplusplus
}
#endif
