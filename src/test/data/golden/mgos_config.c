/* clang-format off */
/*
 * Generated file - do not edit.
 * Command: ../../tools/mgos_gen_config.py --c_name=mgos_config --c_global_name=mgos_sys_config --dest_dir=./build data/sys_conf_wifi.yaml data/sys_conf_http.yaml data/sys_conf_debug.yaml data/sys_conf_overrides.yaml
 */

#include "mgos_config.h"

#include <stdbool.h>

#include "common/cs_file.h"

#include "mgos_config_util.h"


/* struct mgos_config */
static const struct mgos_conf_entry mgos_config_schema_[] = {
    {.type = CONF_TYPE_OBJECT, .key = "", .offset = 0, .num_desc = 43},
    {.type = CONF_TYPE_OBJECT, .key = "wifi", .offset = offsetof(struct mgos_config, wifi), .num_desc = 9},
    {.type = CONF_TYPE_OBJECT, .key = "sta", .offset = offsetof(struct mgos_config, wifi.sta), .num_desc = 2},
    {.type = CONF_TYPE_STRING, .key = "ssid", .offset = offsetof(struct mgos_config, wifi.sta.ssid)},
    {.type = CONF_TYPE_STRING, .key = "pass", .offset = offsetof(struct mgos_config, wifi.sta.pass)},
    {.type = CONF_TYPE_OBJECT, .key = "ap", .offset = offsetof(struct mgos_config, wifi.ap), .num_desc = 5},
    {.type = CONF_TYPE_BOOL, .key = "enable", .offset = offsetof(struct mgos_config, wifi.ap.enable)},
    {.type = CONF_TYPE_STRING, .key = "ssid", .offset = offsetof(struct mgos_config, wifi.ap.ssid)},
    {.type = CONF_TYPE_STRING, .key = "pass", .offset = offsetof(struct mgos_config, wifi.ap.pass)},
    {.type = CONF_TYPE_INT, .key = "channel", .offset = offsetof(struct mgos_config, wifi.ap.channel)},
    {.type = CONF_TYPE_STRING, .key = "dhcp_end", .offset = offsetof(struct mgos_config, wifi.ap.dhcp_end)},
    {.type = CONF_TYPE_INT, .key = "foo", .offset = offsetof(struct mgos_config, foo)},
    {.type = CONF_TYPE_OBJECT, .key = "http", .offset = offsetof(struct mgos_config, http), .num_desc = 2},
    {.type = CONF_TYPE_BOOL, .key = "enable", .offset = offsetof(struct mgos_config, http.enable)},
    {.type = CONF_TYPE_INT, .key = "port", .offset = offsetof(struct mgos_config, http.port)},
    {.type = CONF_TYPE_OBJECT, .key = "debug", .offset = offsetof(struct mgos_config, debug), .num_desc = 11},
    {.type = CONF_TYPE_INT, .key = "level", .offset = offsetof(struct mgos_config, debug.level)},
    {.type = CONF_TYPE_STRING, .key = "dest", .offset = offsetof(struct mgos_config, debug.dest)},
    {.type = CONF_TYPE_STRING, .key = "file_level", .offset = offsetof(struct mgos_config, debug.file_level)},
    {.type = CONF_TYPE_DOUBLE, .key = "test_d1", .offset = offsetof(struct mgos_config, debug.test_d1)},
    {.type = CONF_TYPE_DOUBLE, .key = "test_d2", .offset = offsetof(struct mgos_config, debug.test_d2)},
    {.type = CONF_TYPE_DOUBLE, .key = "test_d3", .offset = offsetof(struct mgos_config, debug.test_d3)},
    {.type = CONF_TYPE_FLOAT, .key = "test_f1", .offset = offsetof(struct mgos_config, debug.test_f1)},
    {.type = CONF_TYPE_FLOAT, .key = "test_f2", .offset = offsetof(struct mgos_config, debug.test_f2)},
    {.type = CONF_TYPE_FLOAT, .key = "test_f3", .offset = offsetof(struct mgos_config, debug.test_f3)},
    {.type = CONF_TYPE_UNSIGNED_INT, .key = "test_ui", .offset = offsetof(struct mgos_config, debug.test_ui)},
    {.type = CONF_TYPE_OBJECT, .key = "empty", .offset = offsetof(struct mgos_config, debug.empty), .num_desc = 0},
    {.type = CONF_TYPE_OBJECT, .key = "test", .offset = offsetof(struct mgos_config, test), .num_desc = 16},
    {.type = CONF_TYPE_OBJECT, .key = "bar1", .offset = offsetof(struct mgos_config, test.bar1), .num_desc = 7},
    {.type = CONF_TYPE_BOOL, .key = "enable", .offset = offsetof(struct mgos_config, test.bar1.enable)},
    {.type = CONF_TYPE_INT, .key = "param1", .offset = offsetof(struct mgos_config, test.bar1.param1)},
    {.type = CONF_TYPE_OBJECT, .key = "inner", .offset = offsetof(struct mgos_config, test.bar1.inner), .num_desc = 2},
    {.type = CONF_TYPE_STRING, .key = "param2", .offset = offsetof(struct mgos_config, test.bar1.inner.param2)},
    {.type = CONF_TYPE_INT, .key = "param3", .offset = offsetof(struct mgos_config, test.bar1.inner.param3)},
    {.type = CONF_TYPE_OBJECT, .key = "baz", .offset = offsetof(struct mgos_config, test.bar1.baz), .num_desc = 1},
    {.type = CONF_TYPE_BOOL, .key = "bazaar", .offset = offsetof(struct mgos_config, test.bar1.baz.bazaar)},
    {.type = CONF_TYPE_OBJECT, .key = "bar2", .offset = offsetof(struct mgos_config, test.bar2), .num_desc = 7},
    {.type = CONF_TYPE_BOOL, .key = "enable", .offset = offsetof(struct mgos_config, test.bar2.enable)},
    {.type = CONF_TYPE_INT, .key = "param1", .offset = offsetof(struct mgos_config, test.bar2.param1)},
    {.type = CONF_TYPE_OBJECT, .key = "inner", .offset = offsetof(struct mgos_config, test.bar2.inner), .num_desc = 2},
    {.type = CONF_TYPE_STRING, .key = "param2", .offset = offsetof(struct mgos_config, test.bar2.inner.param2)},
    {.type = CONF_TYPE_INT, .key = "param3", .offset = offsetof(struct mgos_config, test.bar2.inner.param3)},
    {.type = CONF_TYPE_OBJECT, .key = "baz", .offset = offsetof(struct mgos_config, test.bar2.baz), .num_desc = 1},
    {.type = CONF_TYPE_BOOL, .key = "bazaar", .offset = offsetof(struct mgos_config, test.bar2.baz.bazaar)},
};

/* struct mgos_config_boo */
static const struct mgos_conf_entry mgos_config_boo_schema_[] = {
    {.type = CONF_TYPE_OBJECT, .key = "", .offset = 0, .num_desc = 4},
    {.type = CONF_TYPE_INT, .key = "param5", .offset = offsetof(struct mgos_config_boo, param5)},
    {.type = CONF_TYPE_STRING, .key = "param6", .offset = offsetof(struct mgos_config_boo, param6)},
    {.type = CONF_TYPE_OBJECT, .key = "sub", .offset = offsetof(struct mgos_config_boo, sub), .num_desc = 1},
    {.type = CONF_TYPE_INT, .key = "param7", .offset = offsetof(struct mgos_config_boo, sub.param7)},
};

/* struct mgos_config_wifi_sta */
const struct mgos_conf_entry *mgos_config_wifi_sta_get_schema(void) {
  return &mgos_config_schema_[2];
}

void mgos_config_wifi_sta_set_defaults(struct mgos_config_wifi_sta *cfg) {
  cfg->ssid = NULL;
  cfg->pass = "so\nmany\nlines\n";
}
bool mgos_config_wifi_sta_parse_f(const char *fname, struct mgos_config_wifi_sta *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_wifi_sta_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_wifi_ap */
const struct mgos_conf_entry *mgos_config_wifi_ap_get_schema(void) {
  return &mgos_config_schema_[5];
}

void mgos_config_wifi_ap_set_defaults(struct mgos_config_wifi_ap *cfg) {
  cfg->enable = false;
  cfg->ssid = "Quote \" me \\\\ please";
  cfg->pass = "\u043c\u0430\u043b\u043e\u0432\u0430\u0442\u043e \u0431\u0443\u0434\u0435\u0442";
  cfg->channel = 6;
  cfg->dhcp_end = "192.168.4.200";
}
bool mgos_config_wifi_ap_parse_f(const char *fname, struct mgos_config_wifi_ap *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_wifi_ap_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_wifi */
const struct mgos_conf_entry *mgos_config_wifi_get_schema(void) {
  return &mgos_config_schema_[1];
}

void mgos_config_wifi_set_defaults(struct mgos_config_wifi *cfg) {
  mgos_config_wifi_sta_set_defaults(&cfg->sta);
  mgos_config_wifi_ap_set_defaults(&cfg->ap);
}
bool mgos_config_wifi_parse_f(const char *fname, struct mgos_config_wifi *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_wifi_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_http */
const struct mgos_conf_entry *mgos_config_http_get_schema(void) {
  return &mgos_config_schema_[12];
}

void mgos_config_http_set_defaults(struct mgos_config_http *cfg) {
  cfg->enable = true;
  cfg->port = 80;
}
bool mgos_config_http_parse_f(const char *fname, struct mgos_config_http *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_http_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_debug_empty */
const struct mgos_conf_entry *mgos_config_debug_empty_get_schema(void) {
  return &mgos_config_schema_[26];
}

void mgos_config_debug_empty_set_defaults(struct mgos_config_debug_empty *cfg) {
  (void) cfg;
}
bool mgos_config_debug_empty_parse_f(const char *fname, struct mgos_config_debug_empty *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_debug_empty_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_debug */
const struct mgos_conf_entry *mgos_config_debug_get_schema(void) {
  return &mgos_config_schema_[15];
}

void mgos_config_debug_set_defaults(struct mgos_config_debug *cfg) {
  cfg->level = 2;
  cfg->dest = "uart1";
  cfg->file_level = "mg_foo.c=4";
  cfg->test_d1 = 2.0;
  cfg->test_d2 = 0.0;
  cfg->test_d3 = 0.0001;
  cfg->test_f1 = 0.123;
  cfg->test_f2 = 123.0;
  cfg->test_f3 = 1e-05;
  cfg->test_ui = 4294967295;
  mgos_config_debug_empty_set_defaults(&cfg->empty);
}
bool mgos_config_debug_parse_f(const char *fname, struct mgos_config_debug *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_debug_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_baz */
const struct mgos_conf_entry *mgos_config_baz_get_schema(void) {
  return &mgos_config_schema_[42];
}

void mgos_config_baz_set_defaults(struct mgos_config_baz *cfg) {
  cfg->bazaar = false;
}
bool mgos_config_baz_parse_f(const char *fname, struct mgos_config_baz *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_baz_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_bar_inner */
const struct mgos_conf_entry *mgos_config_bar_inner_get_schema(void) {
  return &mgos_config_schema_[39];
}

void mgos_config_bar_inner_set_defaults(struct mgos_config_bar_inner *cfg) {
  cfg->param2 = "p2";
  cfg->param3 = 3333;
}
bool mgos_config_bar_inner_parse_f(const char *fname, struct mgos_config_bar_inner *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_bar_inner_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_baz */
const struct mgos_conf_entry *mgos_config_bar_baz_get_schema(void) {
  return &mgos_config_schema_[42];
}

void mgos_config_bar_baz_set_defaults(struct mgos_config_baz *cfg) {
  cfg->bazaar = false;
}
bool mgos_config_bar_baz_parse_f(const char *fname, struct mgos_config_baz *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_bar_baz_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_bar */
const struct mgos_conf_entry *mgos_config_bar_get_schema(void) {
  return &mgos_config_schema_[36];
}

void mgos_config_bar_set_defaults(struct mgos_config_bar *cfg) {
  cfg->enable = false;
  cfg->param1 = 1111;
  mgos_config_bar_inner_set_defaults(&cfg->inner);
  mgos_config_bar_baz_set_defaults(&cfg->baz);
}
bool mgos_config_bar_parse_f(const char *fname, struct mgos_config_bar *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_bar_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_bar_inner */
const struct mgos_conf_entry *mgos_config_test_bar1_inner_get_schema(void) {
  return &mgos_config_schema_[39];
}

void mgos_config_test_bar1_inner_set_defaults(struct mgos_config_bar_inner *cfg) {
  cfg->param2 = "p2";
  cfg->param3 = 3333;
}
bool mgos_config_test_bar1_inner_parse_f(const char *fname, struct mgos_config_bar_inner *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_test_bar1_inner_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_baz */
const struct mgos_conf_entry *mgos_config_test_bar1_baz_get_schema(void) {
  return &mgos_config_schema_[42];
}

void mgos_config_test_bar1_baz_set_defaults(struct mgos_config_baz *cfg) {
  cfg->bazaar = false;
}
bool mgos_config_test_bar1_baz_parse_f(const char *fname, struct mgos_config_baz *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_test_bar1_baz_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_bar */
const struct mgos_conf_entry *mgos_config_test_bar1_get_schema(void) {
  return &mgos_config_schema_[36];
}

void mgos_config_test_bar1_set_defaults(struct mgos_config_bar *cfg) {
  cfg->enable = false;
  cfg->param1 = 1111;
  mgos_config_test_bar1_inner_set_defaults(&cfg->inner);
  mgos_config_test_bar1_baz_set_defaults(&cfg->baz);
}
bool mgos_config_test_bar1_parse_f(const char *fname, struct mgos_config_bar *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_test_bar1_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_bar_inner */
const struct mgos_conf_entry *mgos_config_test_bar2_inner_get_schema(void) {
  return &mgos_config_schema_[39];
}

void mgos_config_test_bar2_inner_set_defaults(struct mgos_config_bar_inner *cfg) {
  cfg->param2 = "p2";
  cfg->param3 = 3333;
}
bool mgos_config_test_bar2_inner_parse_f(const char *fname, struct mgos_config_bar_inner *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_test_bar2_inner_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_baz */
const struct mgos_conf_entry *mgos_config_test_bar2_baz_get_schema(void) {
  return &mgos_config_schema_[42];
}

void mgos_config_test_bar2_baz_set_defaults(struct mgos_config_baz *cfg) {
  cfg->bazaar = true;
}
bool mgos_config_test_bar2_baz_parse_f(const char *fname, struct mgos_config_baz *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_test_bar2_baz_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_bar */
const struct mgos_conf_entry *mgos_config_test_bar2_get_schema(void) {
  return &mgos_config_schema_[36];
}

void mgos_config_test_bar2_set_defaults(struct mgos_config_bar *cfg) {
  cfg->enable = false;
  cfg->param1 = 2222;
  mgos_config_test_bar2_inner_set_defaults(&cfg->inner);
  mgos_config_test_bar2_baz_set_defaults(&cfg->baz);
}
bool mgos_config_test_bar2_parse_f(const char *fname, struct mgos_config_bar *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_test_bar2_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_test */
const struct mgos_conf_entry *mgos_config_test_get_schema(void) {
  return &mgos_config_schema_[27];
}

void mgos_config_test_set_defaults(struct mgos_config_test *cfg) {
  mgos_config_test_bar1_set_defaults(&cfg->bar1);
  mgos_config_test_bar2_set_defaults(&cfg->bar2);
}
bool mgos_config_test_parse_f(const char *fname, struct mgos_config_test *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_test_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_boo_sub */
const struct mgos_conf_entry *mgos_config_boo_sub_get_schema(void) {
  return &mgos_config_boo_schema_[3];
}

void mgos_config_boo_sub_set_defaults(struct mgos_config_boo_sub *cfg) {
  cfg->param7 = 444;
}
bool mgos_config_boo_sub_parse_f(const char *fname, struct mgos_config_boo_sub *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_boo_sub_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config_boo */
const struct mgos_conf_entry *mgos_config_boo_get_schema(void) {
  return &mgos_config_boo_schema_[0];
}

void mgos_config_boo_set_defaults(struct mgos_config_boo *cfg) {
  cfg->param5 = 333;
  cfg->param6 = "p6";
  mgos_config_boo_sub_set_defaults(&cfg->sub);
}
bool mgos_config_boo_parse_f(const char *fname, struct mgos_config_boo *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_boo_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* struct mgos_config */
const struct mgos_conf_entry *mgos_config_get_schema(void) {
  return &mgos_config_schema_[0];
}

void mgos_config_set_defaults(struct mgos_config *cfg) {
  mgos_config_wifi_set_defaults(&cfg->wifi);
  cfg->foo = 123;
  mgos_config_http_set_defaults(&cfg->http);
  mgos_config_debug_set_defaults(&cfg->debug);
  mgos_config_test_set_defaults(&cfg->test);
}
bool mgos_config_parse_f(const char *fname, struct mgos_config *cfg) {
  size_t len;
  char *data = cs_read_file(fname, &len);
  if (data == NULL) return false;
  bool res = mgos_config_parse(mg_mk_str_n(data, len), cfg);
  free(data);
  return res;
}

/* Global instance */
struct mgos_config mgos_sys_config;

/* Accessors */

/* wifi */
const struct mgos_config_wifi *mgos_config_get_wifi(const struct mgos_config *cfg) { return &cfg->wifi; }

/* wifi.sta */
const struct mgos_config_wifi_sta *mgos_config_get_wifi_sta(const struct mgos_config *cfg) { return &cfg->wifi.sta; }

/* wifi.sta.ssid */
const char * mgos_config_get_wifi_sta_ssid(const struct mgos_config *cfg) { return cfg->wifi.sta.ssid; }
const char * mgos_config_get_default_wifi_sta_ssid(void) { return NULL; }
void mgos_config_set_wifi_sta_ssid(struct mgos_config *cfg, const char * v) { mgos_conf_set_str(&cfg->wifi.sta.ssid, v); }

/* wifi.sta.pass */
const char * mgos_config_get_wifi_sta_pass(const struct mgos_config *cfg) { return cfg->wifi.sta.pass; }
const char * mgos_config_get_default_wifi_sta_pass(void) { return "so\nmany\nlines\n"; }
void mgos_config_set_wifi_sta_pass(struct mgos_config *cfg, const char * v) { mgos_conf_set_str(&cfg->wifi.sta.pass, v); }

/* wifi.ap */
const struct mgos_config_wifi_ap *mgos_config_get_wifi_ap(const struct mgos_config *cfg) { return &cfg->wifi.ap; }

/* wifi.ap.enable */
int mgos_config_get_wifi_ap_enable(const struct mgos_config *cfg) { return cfg->wifi.ap.enable; }
int mgos_config_get_default_wifi_ap_enable(void) { return false; }
void mgos_config_set_wifi_ap_enable(struct mgos_config *cfg, int v) { cfg->wifi.ap.enable = v; }

/* wifi.ap.ssid */
const char * mgos_config_get_wifi_ap_ssid(const struct mgos_config *cfg) { return cfg->wifi.ap.ssid; }
const char * mgos_config_get_default_wifi_ap_ssid(void) { return "Quote \" me \\\\ please"; }
void mgos_config_set_wifi_ap_ssid(struct mgos_config *cfg, const char * v) { mgos_conf_set_str(&cfg->wifi.ap.ssid, v); }

/* wifi.ap.pass */
const char * mgos_config_get_wifi_ap_pass(const struct mgos_config *cfg) { return cfg->wifi.ap.pass; }
const char * mgos_config_get_default_wifi_ap_pass(void) { return "\u043c\u0430\u043b\u043e\u0432\u0430\u0442\u043e \u0431\u0443\u0434\u0435\u0442"; }
void mgos_config_set_wifi_ap_pass(struct mgos_config *cfg, const char * v) { mgos_conf_set_str(&cfg->wifi.ap.pass, v); }

/* wifi.ap.channel */
int mgos_config_get_wifi_ap_channel(const struct mgos_config *cfg) { return cfg->wifi.ap.channel; }
int mgos_config_get_default_wifi_ap_channel(void) { return 6; }
void mgos_config_set_wifi_ap_channel(struct mgos_config *cfg, int v) { cfg->wifi.ap.channel = v; }

/* wifi.ap.dhcp_end */
const char * mgos_config_get_wifi_ap_dhcp_end(const struct mgos_config *cfg) { return cfg->wifi.ap.dhcp_end; }
const char * mgos_config_get_default_wifi_ap_dhcp_end(void) { return "192.168.4.200"; }
void mgos_config_set_wifi_ap_dhcp_end(struct mgos_config *cfg, const char * v) { mgos_conf_set_str(&cfg->wifi.ap.dhcp_end, v); }

/* foo */
int mgos_config_get_foo(const struct mgos_config *cfg) { return cfg->foo; }
int mgos_config_get_default_foo(void) { return 123; }
void mgos_config_set_foo(struct mgos_config *cfg, int v) { cfg->foo = v; }

/* http */
const struct mgos_config_http *mgos_config_get_http(const struct mgos_config *cfg) { return &cfg->http; }

/* http.enable */
int mgos_config_get_http_enable(const struct mgos_config *cfg) { return cfg->http.enable; }
int mgos_config_get_default_http_enable(void) { return true; }
void mgos_config_set_http_enable(struct mgos_config *cfg, int v) { cfg->http.enable = v; }

/* http.port */
int mgos_config_get_http_port(const struct mgos_config *cfg) { return cfg->http.port; }
int mgos_config_get_default_http_port(void) { return 80; }
void mgos_config_set_http_port(struct mgos_config *cfg, int v) { cfg->http.port = v; }

/* debug */
const struct mgos_config_debug *mgos_config_get_debug(const struct mgos_config *cfg) { return &cfg->debug; }

/* debug.level */
int mgos_config_get_debug_level(const struct mgos_config *cfg) { return cfg->debug.level; }
int mgos_config_get_default_debug_level(void) { return 2; }
void mgos_config_set_debug_level(struct mgos_config *cfg, int v) { cfg->debug.level = v; }

/* debug.dest */
const char * mgos_config_get_debug_dest(const struct mgos_config *cfg) { return cfg->debug.dest; }
const char * mgos_config_get_default_debug_dest(void) { return "uart1"; }
void mgos_config_set_debug_dest(struct mgos_config *cfg, const char * v) { mgos_conf_set_str(&cfg->debug.dest, v); }

/* debug.file_level */
const char * mgos_config_get_debug_file_level(const struct mgos_config *cfg) { return cfg->debug.file_level; }
const char * mgos_config_get_default_debug_file_level(void) { return "mg_foo.c=4"; }
void mgos_config_set_debug_file_level(struct mgos_config *cfg, const char * v) { mgos_conf_set_str(&cfg->debug.file_level, v); }

/* debug.test_d1 */
double mgos_config_get_debug_test_d1(const struct mgos_config *cfg) { return cfg->debug.test_d1; }
double mgos_config_get_default_debug_test_d1(void) { return 2.0; }
void mgos_config_set_debug_test_d1(struct mgos_config *cfg, double v) { cfg->debug.test_d1 = v; }

/* debug.test_d2 */
double mgos_config_get_debug_test_d2(const struct mgos_config *cfg) { return cfg->debug.test_d2; }
double mgos_config_get_default_debug_test_d2(void) { return 0.0; }
void mgos_config_set_debug_test_d2(struct mgos_config *cfg, double v) { cfg->debug.test_d2 = v; }

/* debug.test_d3 */
double mgos_config_get_debug_test_d3(const struct mgos_config *cfg) { return cfg->debug.test_d3; }
double mgos_config_get_default_debug_test_d3(void) { return 0.0001; }
void mgos_config_set_debug_test_d3(struct mgos_config *cfg, double v) { cfg->debug.test_d3 = v; }

/* debug.test_f1 */
float mgos_config_get_debug_test_f1(const struct mgos_config *cfg) { return cfg->debug.test_f1; }
float mgos_config_get_default_debug_test_f1(void) { return 0.123; }
void mgos_config_set_debug_test_f1(struct mgos_config *cfg, float v) { cfg->debug.test_f1 = v; }

/* debug.test_f2 */
float mgos_config_get_debug_test_f2(const struct mgos_config *cfg) { return cfg->debug.test_f2; }
float mgos_config_get_default_debug_test_f2(void) { return 123.0; }
void mgos_config_set_debug_test_f2(struct mgos_config *cfg, float v) { cfg->debug.test_f2 = v; }

/* debug.test_f3 */
float mgos_config_get_debug_test_f3(const struct mgos_config *cfg) { return cfg->debug.test_f3; }
float mgos_config_get_default_debug_test_f3(void) { return 1e-05; }
void mgos_config_set_debug_test_f3(struct mgos_config *cfg, float v) { cfg->debug.test_f3 = v; }

/* debug.test_ui */
unsigned int mgos_config_get_debug_test_ui(const struct mgos_config *cfg) { return cfg->debug.test_ui; }
unsigned int mgos_config_get_default_debug_test_ui(void) { return 4294967295; }
void mgos_config_set_debug_test_ui(struct mgos_config *cfg, unsigned int v) { cfg->debug.test_ui = v; }

/* debug.empty */
const struct mgos_config_debug_empty *mgos_config_get_debug_empty(const struct mgos_config *cfg) { return &cfg->debug.empty; }

/* test */
const struct mgos_config_test *mgos_config_get_test(const struct mgos_config *cfg) { return &cfg->test; }

/* test.bar1 */
const struct mgos_config_bar *mgos_config_get_test_bar1(const struct mgos_config *cfg) { return &cfg->test.bar1; }

/* test.bar1.enable */
int mgos_config_get_test_bar1_enable(const struct mgos_config *cfg) { return cfg->test.bar1.enable; }
int mgos_config_get_default_test_bar1_enable(void) { return false; }
void mgos_config_set_test_bar1_enable(struct mgos_config *cfg, int v) { cfg->test.bar1.enable = v; }

/* test.bar1.param1 */
int mgos_config_get_test_bar1_param1(const struct mgos_config *cfg) { return cfg->test.bar1.param1; }
int mgos_config_get_default_test_bar1_param1(void) { return 1111; }
void mgos_config_set_test_bar1_param1(struct mgos_config *cfg, int v) { cfg->test.bar1.param1 = v; }

/* test.bar1.inner */
const struct mgos_config_bar_inner *mgos_config_get_test_bar1_inner(const struct mgos_config *cfg) { return &cfg->test.bar1.inner; }

/* test.bar1.inner.param2 */
const char * mgos_config_get_test_bar1_inner_param2(const struct mgos_config *cfg) { return cfg->test.bar1.inner.param2; }
const char * mgos_config_get_default_test_bar1_inner_param2(void) { return "p2"; }
void mgos_config_set_test_bar1_inner_param2(struct mgos_config *cfg, const char * v) { mgos_conf_set_str(&cfg->test.bar1.inner.param2, v); }

/* test.bar1.inner.param3 */
int mgos_config_get_test_bar1_inner_param3(const struct mgos_config *cfg) { return cfg->test.bar1.inner.param3; }
int mgos_config_get_default_test_bar1_inner_param3(void) { return 3333; }
void mgos_config_set_test_bar1_inner_param3(struct mgos_config *cfg, int v) { cfg->test.bar1.inner.param3 = v; }

/* test.bar1.baz */
const struct mgos_config_baz *mgos_config_get_test_bar1_baz(const struct mgos_config *cfg) { return &cfg->test.bar1.baz; }

/* test.bar1.baz.bazaar */
int mgos_config_get_test_bar1_baz_bazaar(const struct mgos_config *cfg) { return cfg->test.bar1.baz.bazaar; }
int mgos_config_get_default_test_bar1_baz_bazaar(void) { return false; }
void mgos_config_set_test_bar1_baz_bazaar(struct mgos_config *cfg, int v) { cfg->test.bar1.baz.bazaar = v; }

/* test.bar2 */
const struct mgos_config_bar *mgos_config_get_test_bar2(const struct mgos_config *cfg) { return &cfg->test.bar2; }

/* test.bar2.enable */
int mgos_config_get_test_bar2_enable(const struct mgos_config *cfg) { return cfg->test.bar2.enable; }
int mgos_config_get_default_test_bar2_enable(void) { return false; }
void mgos_config_set_test_bar2_enable(struct mgos_config *cfg, int v) { cfg->test.bar2.enable = v; }

/* test.bar2.param1 */
int mgos_config_get_test_bar2_param1(const struct mgos_config *cfg) { return cfg->test.bar2.param1; }
int mgos_config_get_default_test_bar2_param1(void) { return 2222; }
void mgos_config_set_test_bar2_param1(struct mgos_config *cfg, int v) { cfg->test.bar2.param1 = v; }

/* test.bar2.inner */
const struct mgos_config_bar_inner *mgos_config_get_test_bar2_inner(const struct mgos_config *cfg) { return &cfg->test.bar2.inner; }

/* test.bar2.inner.param2 */
const char * mgos_config_get_test_bar2_inner_param2(const struct mgos_config *cfg) { return cfg->test.bar2.inner.param2; }
const char * mgos_config_get_default_test_bar2_inner_param2(void) { return "p2"; }
void mgos_config_set_test_bar2_inner_param2(struct mgos_config *cfg, const char * v) { mgos_conf_set_str(&cfg->test.bar2.inner.param2, v); }

/* test.bar2.inner.param3 */
int mgos_config_get_test_bar2_inner_param3(const struct mgos_config *cfg) { return cfg->test.bar2.inner.param3; }
int mgos_config_get_default_test_bar2_inner_param3(void) { return 3333; }
void mgos_config_set_test_bar2_inner_param3(struct mgos_config *cfg, int v) { cfg->test.bar2.inner.param3 = v; }

/* test.bar2.baz */
const struct mgos_config_baz *mgos_config_get_test_bar2_baz(const struct mgos_config *cfg) { return &cfg->test.bar2.baz; }

/* test.bar2.baz.bazaar */
int mgos_config_get_test_bar2_baz_bazaar(const struct mgos_config *cfg) { return cfg->test.bar2.baz.bazaar; }
int mgos_config_get_default_test_bar2_baz_bazaar(void) { return true; }
void mgos_config_set_test_bar2_baz_bazaar(struct mgos_config *cfg, int v) { cfg->test.bar2.baz.bazaar = v; }
bool mgos_sys_config_get(const struct mg_str key, struct mg_str *value) {
  return mgos_config_get(key, value, &mgos_sys_config, mgos_config_schema());
}
bool mgos_sys_config_set(const struct mg_str key, const struct mg_str value, bool free_strings) {
  return mgos_config_set(key, value, &mgos_sys_config, mgos_config_schema(), free_strings);
}

const struct mgos_conf_entry *mgos_config_schema(void) {
  return mgos_config_get_schema();
}

/* Strings */
static const char *mgos_config_str_table[] = {
  "192.168.4.200",
  "Quote \" me \\\\ please",
  "mg_foo.c=4",
  "p2",
  "p6",
  "so\nmany\nlines\n",
  "uart1",
  "\u043c\u0430\u043b\u043e\u0432\u0430\u0442\u043e \u0431\u0443\u0434\u0435\u0442",
};

bool mgos_config_is_default_str(const char *s) {
  int num_str = (sizeof(mgos_config_str_table) / sizeof(mgos_config_str_table[0]));
  for (int i = 0; i < num_str; i++) {
    if (mgos_config_str_table[i] == s) return true;
  }
  return false;
}
