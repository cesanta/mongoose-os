/* clang-format off */
/*
 * Generated file - do not edit.
 * Command: ../../tools/mgos_gen_config.py --c_name=mgos_config --c_global_name=mgos_sys_config --dest_dir=./build data/sys_conf_wifi.yaml data/sys_conf_http.yaml data/sys_conf_debug.yaml data/sys_conf_overrides.yaml
 */

#pragma once

#include <stdbool.h>

#include "common/mg_str.h"

#include "mgos_config_util.h"

#ifdef __cplusplus
extern "C" {
#endif


/* wifi.sta type struct mgos_config_wifi_sta */
struct mgos_config_wifi_sta {
  const char * ssid;
  const char * pass;
};
const struct mgos_conf_entry *mgos_config_wifi_sta_get_schema(void);
void mgos_config_wifi_sta_set_defaults(struct mgos_config_wifi_sta *cfg);
static inline bool mgos_config_wifi_sta_parse(struct mg_str json, struct mgos_config_wifi_sta *cfg) {
  mgos_config_wifi_sta_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_wifi_sta_get_schema(), cfg);
}
bool mgos_config_wifi_sta_parse_f(const char *fname, struct mgos_config_wifi_sta *cfg);
static inline bool mgos_config_wifi_sta_emit(const struct mgos_config_wifi_sta *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_wifi_sta_get_schema(), pretty, out);
}
static inline bool mgos_config_wifi_sta_emit_f(const struct mgos_config_wifi_sta *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_wifi_sta_get_schema(), pretty, fname);
}
static inline bool mgos_config_wifi_sta_copy(const struct mgos_config_wifi_sta *src, struct mgos_config_wifi_sta *dst) {
  return mgos_conf_copy(mgos_config_wifi_sta_get_schema(), src, dst);
}
static inline void mgos_config_wifi_sta_free(struct mgos_config_wifi_sta *cfg) {
  return mgos_conf_free(mgos_config_wifi_sta_get_schema(), cfg);
}

/* wifi.ap type struct mgos_config_wifi_ap */
struct mgos_config_wifi_ap {
  int enable;
  const char * ssid;
  const char * pass;
  int channel;
  const char * dhcp_end;
};
const struct mgos_conf_entry *mgos_config_wifi_ap_get_schema(void);
void mgos_config_wifi_ap_set_defaults(struct mgos_config_wifi_ap *cfg);
static inline bool mgos_config_wifi_ap_parse(struct mg_str json, struct mgos_config_wifi_ap *cfg) {
  mgos_config_wifi_ap_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_wifi_ap_get_schema(), cfg);
}
bool mgos_config_wifi_ap_parse_f(const char *fname, struct mgos_config_wifi_ap *cfg);
static inline bool mgos_config_wifi_ap_emit(const struct mgos_config_wifi_ap *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_wifi_ap_get_schema(), pretty, out);
}
static inline bool mgos_config_wifi_ap_emit_f(const struct mgos_config_wifi_ap *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_wifi_ap_get_schema(), pretty, fname);
}
static inline bool mgos_config_wifi_ap_copy(const struct mgos_config_wifi_ap *src, struct mgos_config_wifi_ap *dst) {
  return mgos_conf_copy(mgos_config_wifi_ap_get_schema(), src, dst);
}
static inline void mgos_config_wifi_ap_free(struct mgos_config_wifi_ap *cfg) {
  return mgos_conf_free(mgos_config_wifi_ap_get_schema(), cfg);
}

/* wifi type struct mgos_config_wifi */
struct mgos_config_wifi {
  struct mgos_config_wifi_sta sta;
  struct mgos_config_wifi_ap ap;
};
const struct mgos_conf_entry *mgos_config_wifi_get_schema(void);
void mgos_config_wifi_set_defaults(struct mgos_config_wifi *cfg);
static inline bool mgos_config_wifi_parse(struct mg_str json, struct mgos_config_wifi *cfg) {
  mgos_config_wifi_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_wifi_get_schema(), cfg);
}
bool mgos_config_wifi_parse_f(const char *fname, struct mgos_config_wifi *cfg);
static inline bool mgos_config_wifi_emit(const struct mgos_config_wifi *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_wifi_get_schema(), pretty, out);
}
static inline bool mgos_config_wifi_emit_f(const struct mgos_config_wifi *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_wifi_get_schema(), pretty, fname);
}
static inline bool mgos_config_wifi_copy(const struct mgos_config_wifi *src, struct mgos_config_wifi *dst) {
  return mgos_conf_copy(mgos_config_wifi_get_schema(), src, dst);
}
static inline void mgos_config_wifi_free(struct mgos_config_wifi *cfg) {
  return mgos_conf_free(mgos_config_wifi_get_schema(), cfg);
}

/* http type struct mgos_config_http */
struct mgos_config_http {
  int enable;
  int port;
};
const struct mgos_conf_entry *mgos_config_http_get_schema(void);
void mgos_config_http_set_defaults(struct mgos_config_http *cfg);
static inline bool mgos_config_http_parse(struct mg_str json, struct mgos_config_http *cfg) {
  mgos_config_http_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_http_get_schema(), cfg);
}
bool mgos_config_http_parse_f(const char *fname, struct mgos_config_http *cfg);
static inline bool mgos_config_http_emit(const struct mgos_config_http *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_http_get_schema(), pretty, out);
}
static inline bool mgos_config_http_emit_f(const struct mgos_config_http *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_http_get_schema(), pretty, fname);
}
static inline bool mgos_config_http_copy(const struct mgos_config_http *src, struct mgos_config_http *dst) {
  return mgos_conf_copy(mgos_config_http_get_schema(), src, dst);
}
static inline void mgos_config_http_free(struct mgos_config_http *cfg) {
  return mgos_conf_free(mgos_config_http_get_schema(), cfg);
}

/* debug.empty type struct mgos_config_debug_empty */
struct mgos_config_debug_empty {
};
const struct mgos_conf_entry *mgos_config_debug_empty_get_schema(void);
void mgos_config_debug_empty_set_defaults(struct mgos_config_debug_empty *cfg);
static inline bool mgos_config_debug_empty_parse(struct mg_str json, struct mgos_config_debug_empty *cfg) {
  mgos_config_debug_empty_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_debug_empty_get_schema(), cfg);
}
bool mgos_config_debug_empty_parse_f(const char *fname, struct mgos_config_debug_empty *cfg);
static inline bool mgos_config_debug_empty_emit(const struct mgos_config_debug_empty *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_debug_empty_get_schema(), pretty, out);
}
static inline bool mgos_config_debug_empty_emit_f(const struct mgos_config_debug_empty *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_debug_empty_get_schema(), pretty, fname);
}
static inline bool mgos_config_debug_empty_copy(const struct mgos_config_debug_empty *src, struct mgos_config_debug_empty *dst) {
  return mgos_conf_copy(mgos_config_debug_empty_get_schema(), src, dst);
}
static inline void mgos_config_debug_empty_free(struct mgos_config_debug_empty *cfg) {
  return mgos_conf_free(mgos_config_debug_empty_get_schema(), cfg);
}

/* debug type struct mgos_config_debug */
struct mgos_config_debug {
  int level;
  const char * dest;
  const char * file_level;
  double test_d1;
  double test_d2;
  double test_d3;
  float test_f1;
  float test_f2;
  float test_f3;
  unsigned int test_ui;
  struct mgos_config_debug_empty empty;
};
const struct mgos_conf_entry *mgos_config_debug_get_schema(void);
void mgos_config_debug_set_defaults(struct mgos_config_debug *cfg);
static inline bool mgos_config_debug_parse(struct mg_str json, struct mgos_config_debug *cfg) {
  mgos_config_debug_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_debug_get_schema(), cfg);
}
bool mgos_config_debug_parse_f(const char *fname, struct mgos_config_debug *cfg);
static inline bool mgos_config_debug_emit(const struct mgos_config_debug *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_debug_get_schema(), pretty, out);
}
static inline bool mgos_config_debug_emit_f(const struct mgos_config_debug *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_debug_get_schema(), pretty, fname);
}
static inline bool mgos_config_debug_copy(const struct mgos_config_debug *src, struct mgos_config_debug *dst) {
  return mgos_conf_copy(mgos_config_debug_get_schema(), src, dst);
}
static inline void mgos_config_debug_free(struct mgos_config_debug *cfg) {
  return mgos_conf_free(mgos_config_debug_get_schema(), cfg);
}

/* baz type struct mgos_config_baz */
struct mgos_config_baz {
  int bazaar;
};
const struct mgos_conf_entry *mgos_config_baz_get_schema(void);
void mgos_config_baz_set_defaults(struct mgos_config_baz *cfg);
static inline bool mgos_config_baz_parse(struct mg_str json, struct mgos_config_baz *cfg) {
  mgos_config_baz_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_baz_get_schema(), cfg);
}
bool mgos_config_baz_parse_f(const char *fname, struct mgos_config_baz *cfg);
static inline bool mgos_config_baz_emit(const struct mgos_config_baz *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_baz_get_schema(), pretty, out);
}
static inline bool mgos_config_baz_emit_f(const struct mgos_config_baz *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_baz_get_schema(), pretty, fname);
}
static inline bool mgos_config_baz_copy(const struct mgos_config_baz *src, struct mgos_config_baz *dst) {
  return mgos_conf_copy(mgos_config_baz_get_schema(), src, dst);
}
static inline void mgos_config_baz_free(struct mgos_config_baz *cfg) {
  return mgos_conf_free(mgos_config_baz_get_schema(), cfg);
}

/* bar.inner type struct mgos_config_bar_inner */
struct mgos_config_bar_inner {
  const char * param2;
  int param3;
};
const struct mgos_conf_entry *mgos_config_bar_inner_get_schema(void);
void mgos_config_bar_inner_set_defaults(struct mgos_config_bar_inner *cfg);
static inline bool mgos_config_bar_inner_parse(struct mg_str json, struct mgos_config_bar_inner *cfg) {
  mgos_config_bar_inner_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_bar_inner_get_schema(), cfg);
}
bool mgos_config_bar_inner_parse_f(const char *fname, struct mgos_config_bar_inner *cfg);
static inline bool mgos_config_bar_inner_emit(const struct mgos_config_bar_inner *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_bar_inner_get_schema(), pretty, out);
}
static inline bool mgos_config_bar_inner_emit_f(const struct mgos_config_bar_inner *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_bar_inner_get_schema(), pretty, fname);
}
static inline bool mgos_config_bar_inner_copy(const struct mgos_config_bar_inner *src, struct mgos_config_bar_inner *dst) {
  return mgos_conf_copy(mgos_config_bar_inner_get_schema(), src, dst);
}
static inline void mgos_config_bar_inner_free(struct mgos_config_bar_inner *cfg) {
  return mgos_conf_free(mgos_config_bar_inner_get_schema(), cfg);
}

/* bar.baz type struct mgos_config_baz */
const struct mgos_conf_entry *mgos_config_bar_baz_get_schema(void);
void mgos_config_bar_baz_set_defaults(struct mgos_config_baz *cfg);
static inline bool mgos_config_bar_baz_parse(struct mg_str json, struct mgos_config_baz *cfg) {
  mgos_config_bar_baz_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_bar_baz_get_schema(), cfg);
}
bool mgos_config_bar_baz_parse_f(const char *fname, struct mgos_config_baz *cfg);
static inline bool mgos_config_bar_baz_emit(const struct mgos_config_baz *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_bar_baz_get_schema(), pretty, out);
}
static inline bool mgos_config_bar_baz_emit_f(const struct mgos_config_baz *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_bar_baz_get_schema(), pretty, fname);
}
static inline bool mgos_config_bar_baz_copy(const struct mgos_config_baz *src, struct mgos_config_baz *dst) {
  return mgos_conf_copy(mgos_config_bar_baz_get_schema(), src, dst);
}
static inline void mgos_config_bar_baz_free(struct mgos_config_baz *cfg) {
  return mgos_conf_free(mgos_config_bar_baz_get_schema(), cfg);
}

/* bar type struct mgos_config_bar */
struct mgos_config_bar {
  int enable;
  int param1;
  struct mgos_config_bar_inner inner;
  struct mgos_config_baz baz;
};
const struct mgos_conf_entry *mgos_config_bar_get_schema(void);
void mgos_config_bar_set_defaults(struct mgos_config_bar *cfg);
static inline bool mgos_config_bar_parse(struct mg_str json, struct mgos_config_bar *cfg) {
  mgos_config_bar_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_bar_get_schema(), cfg);
}
bool mgos_config_bar_parse_f(const char *fname, struct mgos_config_bar *cfg);
static inline bool mgos_config_bar_emit(const struct mgos_config_bar *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_bar_get_schema(), pretty, out);
}
static inline bool mgos_config_bar_emit_f(const struct mgos_config_bar *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_bar_get_schema(), pretty, fname);
}
static inline bool mgos_config_bar_copy(const struct mgos_config_bar *src, struct mgos_config_bar *dst) {
  return mgos_conf_copy(mgos_config_bar_get_schema(), src, dst);
}
static inline void mgos_config_bar_free(struct mgos_config_bar *cfg) {
  return mgos_conf_free(mgos_config_bar_get_schema(), cfg);
}

/* test.bar1.inner type struct mgos_config_bar_inner */
const struct mgos_conf_entry *mgos_config_test_bar1_inner_get_schema(void);
void mgos_config_test_bar1_inner_set_defaults(struct mgos_config_bar_inner *cfg);
static inline bool mgos_config_test_bar1_inner_parse(struct mg_str json, struct mgos_config_bar_inner *cfg) {
  mgos_config_test_bar1_inner_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_test_bar1_inner_get_schema(), cfg);
}
bool mgos_config_test_bar1_inner_parse_f(const char *fname, struct mgos_config_bar_inner *cfg);
static inline bool mgos_config_test_bar1_inner_emit(const struct mgos_config_bar_inner *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_test_bar1_inner_get_schema(), pretty, out);
}
static inline bool mgos_config_test_bar1_inner_emit_f(const struct mgos_config_bar_inner *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_test_bar1_inner_get_schema(), pretty, fname);
}
static inline bool mgos_config_test_bar1_inner_copy(const struct mgos_config_bar_inner *src, struct mgos_config_bar_inner *dst) {
  return mgos_conf_copy(mgos_config_test_bar1_inner_get_schema(), src, dst);
}
static inline void mgos_config_test_bar1_inner_free(struct mgos_config_bar_inner *cfg) {
  return mgos_conf_free(mgos_config_test_bar1_inner_get_schema(), cfg);
}

/* test.bar1.baz type struct mgos_config_baz */
const struct mgos_conf_entry *mgos_config_test_bar1_baz_get_schema(void);
void mgos_config_test_bar1_baz_set_defaults(struct mgos_config_baz *cfg);
static inline bool mgos_config_test_bar1_baz_parse(struct mg_str json, struct mgos_config_baz *cfg) {
  mgos_config_test_bar1_baz_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_test_bar1_baz_get_schema(), cfg);
}
bool mgos_config_test_bar1_baz_parse_f(const char *fname, struct mgos_config_baz *cfg);
static inline bool mgos_config_test_bar1_baz_emit(const struct mgos_config_baz *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_test_bar1_baz_get_schema(), pretty, out);
}
static inline bool mgos_config_test_bar1_baz_emit_f(const struct mgos_config_baz *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_test_bar1_baz_get_schema(), pretty, fname);
}
static inline bool mgos_config_test_bar1_baz_copy(const struct mgos_config_baz *src, struct mgos_config_baz *dst) {
  return mgos_conf_copy(mgos_config_test_bar1_baz_get_schema(), src, dst);
}
static inline void mgos_config_test_bar1_baz_free(struct mgos_config_baz *cfg) {
  return mgos_conf_free(mgos_config_test_bar1_baz_get_schema(), cfg);
}

/* test.bar1 type struct mgos_config_bar */
const struct mgos_conf_entry *mgos_config_test_bar1_get_schema(void);
void mgos_config_test_bar1_set_defaults(struct mgos_config_bar *cfg);
static inline bool mgos_config_test_bar1_parse(struct mg_str json, struct mgos_config_bar *cfg) {
  mgos_config_test_bar1_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_test_bar1_get_schema(), cfg);
}
bool mgos_config_test_bar1_parse_f(const char *fname, struct mgos_config_bar *cfg);
static inline bool mgos_config_test_bar1_emit(const struct mgos_config_bar *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_test_bar1_get_schema(), pretty, out);
}
static inline bool mgos_config_test_bar1_emit_f(const struct mgos_config_bar *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_test_bar1_get_schema(), pretty, fname);
}
static inline bool mgos_config_test_bar1_copy(const struct mgos_config_bar *src, struct mgos_config_bar *dst) {
  return mgos_conf_copy(mgos_config_test_bar1_get_schema(), src, dst);
}
static inline void mgos_config_test_bar1_free(struct mgos_config_bar *cfg) {
  return mgos_conf_free(mgos_config_test_bar1_get_schema(), cfg);
}

/* test.bar2.inner type struct mgos_config_bar_inner */
const struct mgos_conf_entry *mgos_config_test_bar2_inner_get_schema(void);
void mgos_config_test_bar2_inner_set_defaults(struct mgos_config_bar_inner *cfg);
static inline bool mgos_config_test_bar2_inner_parse(struct mg_str json, struct mgos_config_bar_inner *cfg) {
  mgos_config_test_bar2_inner_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_test_bar2_inner_get_schema(), cfg);
}
bool mgos_config_test_bar2_inner_parse_f(const char *fname, struct mgos_config_bar_inner *cfg);
static inline bool mgos_config_test_bar2_inner_emit(const struct mgos_config_bar_inner *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_test_bar2_inner_get_schema(), pretty, out);
}
static inline bool mgos_config_test_bar2_inner_emit_f(const struct mgos_config_bar_inner *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_test_bar2_inner_get_schema(), pretty, fname);
}
static inline bool mgos_config_test_bar2_inner_copy(const struct mgos_config_bar_inner *src, struct mgos_config_bar_inner *dst) {
  return mgos_conf_copy(mgos_config_test_bar2_inner_get_schema(), src, dst);
}
static inline void mgos_config_test_bar2_inner_free(struct mgos_config_bar_inner *cfg) {
  return mgos_conf_free(mgos_config_test_bar2_inner_get_schema(), cfg);
}

/* test.bar2.baz type struct mgos_config_baz */
const struct mgos_conf_entry *mgos_config_test_bar2_baz_get_schema(void);
void mgos_config_test_bar2_baz_set_defaults(struct mgos_config_baz *cfg);
static inline bool mgos_config_test_bar2_baz_parse(struct mg_str json, struct mgos_config_baz *cfg) {
  mgos_config_test_bar2_baz_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_test_bar2_baz_get_schema(), cfg);
}
bool mgos_config_test_bar2_baz_parse_f(const char *fname, struct mgos_config_baz *cfg);
static inline bool mgos_config_test_bar2_baz_emit(const struct mgos_config_baz *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_test_bar2_baz_get_schema(), pretty, out);
}
static inline bool mgos_config_test_bar2_baz_emit_f(const struct mgos_config_baz *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_test_bar2_baz_get_schema(), pretty, fname);
}
static inline bool mgos_config_test_bar2_baz_copy(const struct mgos_config_baz *src, struct mgos_config_baz *dst) {
  return mgos_conf_copy(mgos_config_test_bar2_baz_get_schema(), src, dst);
}
static inline void mgos_config_test_bar2_baz_free(struct mgos_config_baz *cfg) {
  return mgos_conf_free(mgos_config_test_bar2_baz_get_schema(), cfg);
}

/* test.bar2 type struct mgos_config_bar */
const struct mgos_conf_entry *mgos_config_test_bar2_get_schema(void);
void mgos_config_test_bar2_set_defaults(struct mgos_config_bar *cfg);
static inline bool mgos_config_test_bar2_parse(struct mg_str json, struct mgos_config_bar *cfg) {
  mgos_config_test_bar2_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_test_bar2_get_schema(), cfg);
}
bool mgos_config_test_bar2_parse_f(const char *fname, struct mgos_config_bar *cfg);
static inline bool mgos_config_test_bar2_emit(const struct mgos_config_bar *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_test_bar2_get_schema(), pretty, out);
}
static inline bool mgos_config_test_bar2_emit_f(const struct mgos_config_bar *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_test_bar2_get_schema(), pretty, fname);
}
static inline bool mgos_config_test_bar2_copy(const struct mgos_config_bar *src, struct mgos_config_bar *dst) {
  return mgos_conf_copy(mgos_config_test_bar2_get_schema(), src, dst);
}
static inline void mgos_config_test_bar2_free(struct mgos_config_bar *cfg) {
  return mgos_conf_free(mgos_config_test_bar2_get_schema(), cfg);
}

/* test type struct mgos_config_test */
struct mgos_config_test {
  struct mgos_config_bar bar1;
  struct mgos_config_bar bar2;
};
const struct mgos_conf_entry *mgos_config_test_get_schema(void);
void mgos_config_test_set_defaults(struct mgos_config_test *cfg);
static inline bool mgos_config_test_parse(struct mg_str json, struct mgos_config_test *cfg) {
  mgos_config_test_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_test_get_schema(), cfg);
}
bool mgos_config_test_parse_f(const char *fname, struct mgos_config_test *cfg);
static inline bool mgos_config_test_emit(const struct mgos_config_test *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_test_get_schema(), pretty, out);
}
static inline bool mgos_config_test_emit_f(const struct mgos_config_test *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_test_get_schema(), pretty, fname);
}
static inline bool mgos_config_test_copy(const struct mgos_config_test *src, struct mgos_config_test *dst) {
  return mgos_conf_copy(mgos_config_test_get_schema(), src, dst);
}
static inline void mgos_config_test_free(struct mgos_config_test *cfg) {
  return mgos_conf_free(mgos_config_test_get_schema(), cfg);
}

/* boo.sub type struct mgos_config_boo_sub */
struct mgos_config_boo_sub {
  int param7;
};
const struct mgos_conf_entry *mgos_config_boo_sub_get_schema(void);
void mgos_config_boo_sub_set_defaults(struct mgos_config_boo_sub *cfg);
static inline bool mgos_config_boo_sub_parse(struct mg_str json, struct mgos_config_boo_sub *cfg) {
  mgos_config_boo_sub_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_boo_sub_get_schema(), cfg);
}
bool mgos_config_boo_sub_parse_f(const char *fname, struct mgos_config_boo_sub *cfg);
static inline bool mgos_config_boo_sub_emit(const struct mgos_config_boo_sub *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_boo_sub_get_schema(), pretty, out);
}
static inline bool mgos_config_boo_sub_emit_f(const struct mgos_config_boo_sub *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_boo_sub_get_schema(), pretty, fname);
}
static inline bool mgos_config_boo_sub_copy(const struct mgos_config_boo_sub *src, struct mgos_config_boo_sub *dst) {
  return mgos_conf_copy(mgos_config_boo_sub_get_schema(), src, dst);
}
static inline void mgos_config_boo_sub_free(struct mgos_config_boo_sub *cfg) {
  return mgos_conf_free(mgos_config_boo_sub_get_schema(), cfg);
}

/* boo type struct mgos_config_boo */
struct mgos_config_boo {
  int param5;
  const char * param6;
  struct mgos_config_boo_sub sub;
};
const struct mgos_conf_entry *mgos_config_boo_get_schema(void);
void mgos_config_boo_set_defaults(struct mgos_config_boo *cfg);
static inline bool mgos_config_boo_parse(struct mg_str json, struct mgos_config_boo *cfg) {
  mgos_config_boo_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_boo_get_schema(), cfg);
}
bool mgos_config_boo_parse_f(const char *fname, struct mgos_config_boo *cfg);
static inline bool mgos_config_boo_emit(const struct mgos_config_boo *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_boo_get_schema(), pretty, out);
}
static inline bool mgos_config_boo_emit_f(const struct mgos_config_boo *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_boo_get_schema(), pretty, fname);
}
static inline bool mgos_config_boo_copy(const struct mgos_config_boo *src, struct mgos_config_boo *dst) {
  return mgos_conf_copy(mgos_config_boo_get_schema(), src, dst);
}
static inline void mgos_config_boo_free(struct mgos_config_boo *cfg) {
  return mgos_conf_free(mgos_config_boo_get_schema(), cfg);
}

/* <root> type struct mgos_config */
struct mgos_config {
  struct mgos_config_wifi wifi;
  int foo;
  struct mgos_config_http http;
  struct mgos_config_debug debug;
  struct mgos_config_test test;
};
const struct mgos_conf_entry *mgos_config_get_schema(void);
void mgos_config_set_defaults(struct mgos_config *cfg);
static inline bool mgos_config_parse(struct mg_str json, struct mgos_config *cfg) {
  mgos_config_set_defaults(cfg);
  return mgos_conf_parse_sub(json, mgos_config_get_schema(), cfg);
}
bool mgos_config_parse_f(const char *fname, struct mgos_config *cfg);
static inline bool mgos_config_emit(const struct mgos_config *cfg, bool pretty, struct json_out *out) {
  return mgos_conf_emit_json_out(cfg, NULL, mgos_config_get_schema(), pretty, out);
}
static inline bool mgos_config_emit_f(const struct mgos_config *cfg, bool pretty, const char *fname) {
  return mgos_conf_emit_f(cfg, NULL, mgos_config_get_schema(), pretty, fname);
}
static inline bool mgos_config_copy(const struct mgos_config *src, struct mgos_config *dst) {
  return mgos_conf_copy(mgos_config_get_schema(), src, dst);
}
static inline void mgos_config_free(struct mgos_config *cfg) {
  return mgos_conf_free(mgos_config_get_schema(), cfg);
}

extern struct mgos_config mgos_sys_config;

/* wifi */
#define MGOS_CONFIG_HAVE_WIFI
#define MGOS_SYS_CONFIG_HAVE_WIFI
const struct mgos_config_wifi *mgos_config_get_wifi(const struct mgos_config *cfg);
static inline const struct mgos_config_wifi *mgos_sys_config_get_wifi(void) { return mgos_config_get_wifi(&mgos_sys_config); }

/* wifi.sta */
#define MGOS_CONFIG_HAVE_WIFI_STA
#define MGOS_SYS_CONFIG_HAVE_WIFI_STA
const struct mgos_config_wifi_sta *mgos_config_get_wifi_sta(const struct mgos_config *cfg);
static inline const struct mgos_config_wifi_sta *mgos_sys_config_get_wifi_sta(void) { return mgos_config_get_wifi_sta(&mgos_sys_config); }

/* wifi.sta.ssid */
#define MGOS_CONFIG_HAVE_WIFI_STA_SSID
#define MGOS_SYS_CONFIG_HAVE_WIFI_STA_SSID
const char * mgos_config_get_wifi_sta_ssid(const struct mgos_config *cfg);
const char * mgos_config_get_default_wifi_sta_ssid(void);
static inline const char * mgos_sys_config_get_wifi_sta_ssid(void) { return mgos_config_get_wifi_sta_ssid(&mgos_sys_config); }
static inline const char * mgos_sys_config_get_default_wifi_sta_ssid(void) { return mgos_config_get_default_wifi_sta_ssid(); }
void mgos_config_set_wifi_sta_ssid(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_wifi_sta_ssid(const char * v) { mgos_config_set_wifi_sta_ssid(&mgos_sys_config, v); }

/* wifi.sta.pass */
#define MGOS_CONFIG_HAVE_WIFI_STA_PASS
#define MGOS_SYS_CONFIG_HAVE_WIFI_STA_PASS
const char * mgos_config_get_wifi_sta_pass(const struct mgos_config *cfg);
const char * mgos_config_get_default_wifi_sta_pass(void);
static inline const char * mgos_sys_config_get_wifi_sta_pass(void) { return mgos_config_get_wifi_sta_pass(&mgos_sys_config); }
static inline const char * mgos_sys_config_get_default_wifi_sta_pass(void) { return mgos_config_get_default_wifi_sta_pass(); }
void mgos_config_set_wifi_sta_pass(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_wifi_sta_pass(const char * v) { mgos_config_set_wifi_sta_pass(&mgos_sys_config, v); }

/* wifi.ap */
#define MGOS_CONFIG_HAVE_WIFI_AP
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP
const struct mgos_config_wifi_ap *mgos_config_get_wifi_ap(const struct mgos_config *cfg);
static inline const struct mgos_config_wifi_ap *mgos_sys_config_get_wifi_ap(void) { return mgos_config_get_wifi_ap(&mgos_sys_config); }

/* wifi.ap.enable */
#define MGOS_CONFIG_HAVE_WIFI_AP_ENABLE
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_ENABLE
int mgos_config_get_wifi_ap_enable(const struct mgos_config *cfg);
int mgos_config_get_default_wifi_ap_enable(void);
static inline int mgos_sys_config_get_wifi_ap_enable(void) { return mgos_config_get_wifi_ap_enable(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_wifi_ap_enable(void) { return mgos_config_get_default_wifi_ap_enable(); }
void mgos_config_set_wifi_ap_enable(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_wifi_ap_enable(int v) { mgos_config_set_wifi_ap_enable(&mgos_sys_config, v); }

/* wifi.ap.ssid */
#define MGOS_CONFIG_HAVE_WIFI_AP_SSID
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_SSID
const char * mgos_config_get_wifi_ap_ssid(const struct mgos_config *cfg);
const char * mgos_config_get_default_wifi_ap_ssid(void);
static inline const char * mgos_sys_config_get_wifi_ap_ssid(void) { return mgos_config_get_wifi_ap_ssid(&mgos_sys_config); }
static inline const char * mgos_sys_config_get_default_wifi_ap_ssid(void) { return mgos_config_get_default_wifi_ap_ssid(); }
void mgos_config_set_wifi_ap_ssid(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_wifi_ap_ssid(const char * v) { mgos_config_set_wifi_ap_ssid(&mgos_sys_config, v); }

/* wifi.ap.pass */
#define MGOS_CONFIG_HAVE_WIFI_AP_PASS
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_PASS
const char * mgos_config_get_wifi_ap_pass(const struct mgos_config *cfg);
const char * mgos_config_get_default_wifi_ap_pass(void);
static inline const char * mgos_sys_config_get_wifi_ap_pass(void) { return mgos_config_get_wifi_ap_pass(&mgos_sys_config); }
static inline const char * mgos_sys_config_get_default_wifi_ap_pass(void) { return mgos_config_get_default_wifi_ap_pass(); }
void mgos_config_set_wifi_ap_pass(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_wifi_ap_pass(const char * v) { mgos_config_set_wifi_ap_pass(&mgos_sys_config, v); }

/* wifi.ap.channel */
#define MGOS_CONFIG_HAVE_WIFI_AP_CHANNEL
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_CHANNEL
int mgos_config_get_wifi_ap_channel(const struct mgos_config *cfg);
int mgos_config_get_default_wifi_ap_channel(void);
static inline int mgos_sys_config_get_wifi_ap_channel(void) { return mgos_config_get_wifi_ap_channel(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_wifi_ap_channel(void) { return mgos_config_get_default_wifi_ap_channel(); }
void mgos_config_set_wifi_ap_channel(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_wifi_ap_channel(int v) { mgos_config_set_wifi_ap_channel(&mgos_sys_config, v); }

/* wifi.ap.dhcp_end */
#define MGOS_CONFIG_HAVE_WIFI_AP_DHCP_END
#define MGOS_SYS_CONFIG_HAVE_WIFI_AP_DHCP_END
const char * mgos_config_get_wifi_ap_dhcp_end(const struct mgos_config *cfg);
const char * mgos_config_get_default_wifi_ap_dhcp_end(void);
static inline const char * mgos_sys_config_get_wifi_ap_dhcp_end(void) { return mgos_config_get_wifi_ap_dhcp_end(&mgos_sys_config); }
static inline const char * mgos_sys_config_get_default_wifi_ap_dhcp_end(void) { return mgos_config_get_default_wifi_ap_dhcp_end(); }
void mgos_config_set_wifi_ap_dhcp_end(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_wifi_ap_dhcp_end(const char * v) { mgos_config_set_wifi_ap_dhcp_end(&mgos_sys_config, v); }

/* foo */
#define MGOS_CONFIG_HAVE_FOO
#define MGOS_SYS_CONFIG_HAVE_FOO
int mgos_config_get_foo(const struct mgos_config *cfg);
int mgos_config_get_default_foo(void);
static inline int mgos_sys_config_get_foo(void) { return mgos_config_get_foo(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_foo(void) { return mgos_config_get_default_foo(); }
void mgos_config_set_foo(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_foo(int v) { mgos_config_set_foo(&mgos_sys_config, v); }

/* http */
#define MGOS_CONFIG_HAVE_HTTP
#define MGOS_SYS_CONFIG_HAVE_HTTP
const struct mgos_config_http *mgos_config_get_http(const struct mgos_config *cfg);
static inline const struct mgos_config_http *mgos_sys_config_get_http(void) { return mgos_config_get_http(&mgos_sys_config); }

/* http.enable */
#define MGOS_CONFIG_HAVE_HTTP_ENABLE
#define MGOS_SYS_CONFIG_HAVE_HTTP_ENABLE
int mgos_config_get_http_enable(const struct mgos_config *cfg);
int mgos_config_get_default_http_enable(void);
static inline int mgos_sys_config_get_http_enable(void) { return mgos_config_get_http_enable(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_http_enable(void) { return mgos_config_get_default_http_enable(); }
void mgos_config_set_http_enable(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_http_enable(int v) { mgos_config_set_http_enable(&mgos_sys_config, v); }

/* http.port */
#define MGOS_CONFIG_HAVE_HTTP_PORT
#define MGOS_SYS_CONFIG_HAVE_HTTP_PORT
int mgos_config_get_http_port(const struct mgos_config *cfg);
int mgos_config_get_default_http_port(void);
static inline int mgos_sys_config_get_http_port(void) { return mgos_config_get_http_port(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_http_port(void) { return mgos_config_get_default_http_port(); }
void mgos_config_set_http_port(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_http_port(int v) { mgos_config_set_http_port(&mgos_sys_config, v); }

/* debug */
#define MGOS_CONFIG_HAVE_DEBUG
#define MGOS_SYS_CONFIG_HAVE_DEBUG
const struct mgos_config_debug *mgos_config_get_debug(const struct mgos_config *cfg);
static inline const struct mgos_config_debug *mgos_sys_config_get_debug(void) { return mgos_config_get_debug(&mgos_sys_config); }

/* debug.level */
#define MGOS_CONFIG_HAVE_DEBUG_LEVEL
#define MGOS_SYS_CONFIG_HAVE_DEBUG_LEVEL
int mgos_config_get_debug_level(const struct mgos_config *cfg);
int mgos_config_get_default_debug_level(void);
static inline int mgos_sys_config_get_debug_level(void) { return mgos_config_get_debug_level(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_debug_level(void) { return mgos_config_get_default_debug_level(); }
void mgos_config_set_debug_level(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_debug_level(int v) { mgos_config_set_debug_level(&mgos_sys_config, v); }

/* debug.dest */
#define MGOS_CONFIG_HAVE_DEBUG_DEST
#define MGOS_SYS_CONFIG_HAVE_DEBUG_DEST
const char * mgos_config_get_debug_dest(const struct mgos_config *cfg);
const char * mgos_config_get_default_debug_dest(void);
static inline const char * mgos_sys_config_get_debug_dest(void) { return mgos_config_get_debug_dest(&mgos_sys_config); }
static inline const char * mgos_sys_config_get_default_debug_dest(void) { return mgos_config_get_default_debug_dest(); }
void mgos_config_set_debug_dest(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_debug_dest(const char * v) { mgos_config_set_debug_dest(&mgos_sys_config, v); }

/* debug.file_level */
#define MGOS_CONFIG_HAVE_DEBUG_FILE_LEVEL
#define MGOS_SYS_CONFIG_HAVE_DEBUG_FILE_LEVEL
const char * mgos_config_get_debug_file_level(const struct mgos_config *cfg);
const char * mgos_config_get_default_debug_file_level(void);
static inline const char * mgos_sys_config_get_debug_file_level(void) { return mgos_config_get_debug_file_level(&mgos_sys_config); }
static inline const char * mgos_sys_config_get_default_debug_file_level(void) { return mgos_config_get_default_debug_file_level(); }
void mgos_config_set_debug_file_level(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_debug_file_level(const char * v) { mgos_config_set_debug_file_level(&mgos_sys_config, v); }

/* debug.test_d1 */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_D1
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_D1
double mgos_config_get_debug_test_d1(const struct mgos_config *cfg);
double mgos_config_get_default_debug_test_d1(void);
static inline double mgos_sys_config_get_debug_test_d1(void) { return mgos_config_get_debug_test_d1(&mgos_sys_config); }
static inline double mgos_sys_config_get_default_debug_test_d1(void) { return mgos_config_get_default_debug_test_d1(); }
void mgos_config_set_debug_test_d1(struct mgos_config *cfg, double v);
static inline void mgos_sys_config_set_debug_test_d1(double v) { mgos_config_set_debug_test_d1(&mgos_sys_config, v); }

/* debug.test_d2 */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_D2
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_D2
double mgos_config_get_debug_test_d2(const struct mgos_config *cfg);
double mgos_config_get_default_debug_test_d2(void);
static inline double mgos_sys_config_get_debug_test_d2(void) { return mgos_config_get_debug_test_d2(&mgos_sys_config); }
static inline double mgos_sys_config_get_default_debug_test_d2(void) { return mgos_config_get_default_debug_test_d2(); }
void mgos_config_set_debug_test_d2(struct mgos_config *cfg, double v);
static inline void mgos_sys_config_set_debug_test_d2(double v) { mgos_config_set_debug_test_d2(&mgos_sys_config, v); }

/* debug.test_d3 */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_D3
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_D3
double mgos_config_get_debug_test_d3(const struct mgos_config *cfg);
double mgos_config_get_default_debug_test_d3(void);
static inline double mgos_sys_config_get_debug_test_d3(void) { return mgos_config_get_debug_test_d3(&mgos_sys_config); }
static inline double mgos_sys_config_get_default_debug_test_d3(void) { return mgos_config_get_default_debug_test_d3(); }
void mgos_config_set_debug_test_d3(struct mgos_config *cfg, double v);
static inline void mgos_sys_config_set_debug_test_d3(double v) { mgos_config_set_debug_test_d3(&mgos_sys_config, v); }

/* debug.test_f1 */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_F1
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_F1
float mgos_config_get_debug_test_f1(const struct mgos_config *cfg);
float mgos_config_get_default_debug_test_f1(void);
static inline float mgos_sys_config_get_debug_test_f1(void) { return mgos_config_get_debug_test_f1(&mgos_sys_config); }
static inline float mgos_sys_config_get_default_debug_test_f1(void) { return mgos_config_get_default_debug_test_f1(); }
void mgos_config_set_debug_test_f1(struct mgos_config *cfg, float v);
static inline void mgos_sys_config_set_debug_test_f1(float v) { mgos_config_set_debug_test_f1(&mgos_sys_config, v); }

/* debug.test_f2 */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_F2
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_F2
float mgos_config_get_debug_test_f2(const struct mgos_config *cfg);
float mgos_config_get_default_debug_test_f2(void);
static inline float mgos_sys_config_get_debug_test_f2(void) { return mgos_config_get_debug_test_f2(&mgos_sys_config); }
static inline float mgos_sys_config_get_default_debug_test_f2(void) { return mgos_config_get_default_debug_test_f2(); }
void mgos_config_set_debug_test_f2(struct mgos_config *cfg, float v);
static inline void mgos_sys_config_set_debug_test_f2(float v) { mgos_config_set_debug_test_f2(&mgos_sys_config, v); }

/* debug.test_f3 */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_F3
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_F3
float mgos_config_get_debug_test_f3(const struct mgos_config *cfg);
float mgos_config_get_default_debug_test_f3(void);
static inline float mgos_sys_config_get_debug_test_f3(void) { return mgos_config_get_debug_test_f3(&mgos_sys_config); }
static inline float mgos_sys_config_get_default_debug_test_f3(void) { return mgos_config_get_default_debug_test_f3(); }
void mgos_config_set_debug_test_f3(struct mgos_config *cfg, float v);
static inline void mgos_sys_config_set_debug_test_f3(float v) { mgos_config_set_debug_test_f3(&mgos_sys_config, v); }

/* debug.test_ui */
#define MGOS_CONFIG_HAVE_DEBUG_TEST_UI
#define MGOS_SYS_CONFIG_HAVE_DEBUG_TEST_UI
unsigned int mgos_config_get_debug_test_ui(const struct mgos_config *cfg);
unsigned int mgos_config_get_default_debug_test_ui(void);
static inline unsigned int mgos_sys_config_get_debug_test_ui(void) { return mgos_config_get_debug_test_ui(&mgos_sys_config); }
static inline unsigned int mgos_sys_config_get_default_debug_test_ui(void) { return mgos_config_get_default_debug_test_ui(); }
void mgos_config_set_debug_test_ui(struct mgos_config *cfg, unsigned int v);
static inline void mgos_sys_config_set_debug_test_ui(unsigned int v) { mgos_config_set_debug_test_ui(&mgos_sys_config, v); }

/* debug.empty */
#define MGOS_CONFIG_HAVE_DEBUG_EMPTY
#define MGOS_SYS_CONFIG_HAVE_DEBUG_EMPTY
const struct mgos_config_debug_empty *mgos_config_get_debug_empty(const struct mgos_config *cfg);
static inline const struct mgos_config_debug_empty *mgos_sys_config_get_debug_empty(void) { return mgos_config_get_debug_empty(&mgos_sys_config); }

/* test */
#define MGOS_CONFIG_HAVE_TEST
#define MGOS_SYS_CONFIG_HAVE_TEST
const struct mgos_config_test *mgos_config_get_test(const struct mgos_config *cfg);
static inline const struct mgos_config_test *mgos_sys_config_get_test(void) { return mgos_config_get_test(&mgos_sys_config); }

/* test.bar1 */
#define MGOS_CONFIG_HAVE_TEST_BAR1
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1
const struct mgos_config_bar *mgos_config_get_test_bar1(const struct mgos_config *cfg);
static inline const struct mgos_config_bar *mgos_sys_config_get_test_bar1(void) { return mgos_config_get_test_bar1(&mgos_sys_config); }

/* test.bar1.enable */
#define MGOS_CONFIG_HAVE_TEST_BAR1_ENABLE
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1_ENABLE
int mgos_config_get_test_bar1_enable(const struct mgos_config *cfg);
int mgos_config_get_default_test_bar1_enable(void);
static inline int mgos_sys_config_get_test_bar1_enable(void) { return mgos_config_get_test_bar1_enable(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_test_bar1_enable(void) { return mgos_config_get_default_test_bar1_enable(); }
void mgos_config_set_test_bar1_enable(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_test_bar1_enable(int v) { mgos_config_set_test_bar1_enable(&mgos_sys_config, v); }

/* test.bar1.param1 */
#define MGOS_CONFIG_HAVE_TEST_BAR1_PARAM1
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1_PARAM1
int mgos_config_get_test_bar1_param1(const struct mgos_config *cfg);
int mgos_config_get_default_test_bar1_param1(void);
static inline int mgos_sys_config_get_test_bar1_param1(void) { return mgos_config_get_test_bar1_param1(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_test_bar1_param1(void) { return mgos_config_get_default_test_bar1_param1(); }
void mgos_config_set_test_bar1_param1(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_test_bar1_param1(int v) { mgos_config_set_test_bar1_param1(&mgos_sys_config, v); }

/* test.bar1.inner */
#define MGOS_CONFIG_HAVE_TEST_BAR1_INNER
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1_INNER
const struct mgos_config_bar_inner *mgos_config_get_test_bar1_inner(const struct mgos_config *cfg);
static inline const struct mgos_config_bar_inner *mgos_sys_config_get_test_bar1_inner(void) { return mgos_config_get_test_bar1_inner(&mgos_sys_config); }

/* test.bar1.inner.param2 */
#define MGOS_CONFIG_HAVE_TEST_BAR1_INNER_PARAM2
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1_INNER_PARAM2
const char * mgos_config_get_test_bar1_inner_param2(const struct mgos_config *cfg);
const char * mgos_config_get_default_test_bar1_inner_param2(void);
static inline const char * mgos_sys_config_get_test_bar1_inner_param2(void) { return mgos_config_get_test_bar1_inner_param2(&mgos_sys_config); }
static inline const char * mgos_sys_config_get_default_test_bar1_inner_param2(void) { return mgos_config_get_default_test_bar1_inner_param2(); }
void mgos_config_set_test_bar1_inner_param2(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_test_bar1_inner_param2(const char * v) { mgos_config_set_test_bar1_inner_param2(&mgos_sys_config, v); }

/* test.bar1.inner.param3 */
#define MGOS_CONFIG_HAVE_TEST_BAR1_INNER_PARAM3
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1_INNER_PARAM3
int mgos_config_get_test_bar1_inner_param3(const struct mgos_config *cfg);
int mgos_config_get_default_test_bar1_inner_param3(void);
static inline int mgos_sys_config_get_test_bar1_inner_param3(void) { return mgos_config_get_test_bar1_inner_param3(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_test_bar1_inner_param3(void) { return mgos_config_get_default_test_bar1_inner_param3(); }
void mgos_config_set_test_bar1_inner_param3(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_test_bar1_inner_param3(int v) { mgos_config_set_test_bar1_inner_param3(&mgos_sys_config, v); }

/* test.bar1.baz */
#define MGOS_CONFIG_HAVE_TEST_BAR1_BAZ
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1_BAZ
const struct mgos_config_baz *mgos_config_get_test_bar1_baz(const struct mgos_config *cfg);
static inline const struct mgos_config_baz *mgos_sys_config_get_test_bar1_baz(void) { return mgos_config_get_test_bar1_baz(&mgos_sys_config); }

/* test.bar1.baz.bazaar */
#define MGOS_CONFIG_HAVE_TEST_BAR1_BAZ_BAZAAR
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR1_BAZ_BAZAAR
int mgos_config_get_test_bar1_baz_bazaar(const struct mgos_config *cfg);
int mgos_config_get_default_test_bar1_baz_bazaar(void);
static inline int mgos_sys_config_get_test_bar1_baz_bazaar(void) { return mgos_config_get_test_bar1_baz_bazaar(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_test_bar1_baz_bazaar(void) { return mgos_config_get_default_test_bar1_baz_bazaar(); }
void mgos_config_set_test_bar1_baz_bazaar(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_test_bar1_baz_bazaar(int v) { mgos_config_set_test_bar1_baz_bazaar(&mgos_sys_config, v); }

/* test.bar2 */
#define MGOS_CONFIG_HAVE_TEST_BAR2
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR2
const struct mgos_config_bar *mgos_config_get_test_bar2(const struct mgos_config *cfg);
static inline const struct mgos_config_bar *mgos_sys_config_get_test_bar2(void) { return mgos_config_get_test_bar2(&mgos_sys_config); }

/* test.bar2.enable */
#define MGOS_CONFIG_HAVE_TEST_BAR2_ENABLE
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR2_ENABLE
int mgos_config_get_test_bar2_enable(const struct mgos_config *cfg);
int mgos_config_get_default_test_bar2_enable(void);
static inline int mgos_sys_config_get_test_bar2_enable(void) { return mgos_config_get_test_bar2_enable(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_test_bar2_enable(void) { return mgos_config_get_default_test_bar2_enable(); }
void mgos_config_set_test_bar2_enable(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_test_bar2_enable(int v) { mgos_config_set_test_bar2_enable(&mgos_sys_config, v); }

/* test.bar2.param1 */
#define MGOS_CONFIG_HAVE_TEST_BAR2_PARAM1
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR2_PARAM1
int mgos_config_get_test_bar2_param1(const struct mgos_config *cfg);
int mgos_config_get_default_test_bar2_param1(void);
static inline int mgos_sys_config_get_test_bar2_param1(void) { return mgos_config_get_test_bar2_param1(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_test_bar2_param1(void) { return mgos_config_get_default_test_bar2_param1(); }
void mgos_config_set_test_bar2_param1(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_test_bar2_param1(int v) { mgos_config_set_test_bar2_param1(&mgos_sys_config, v); }

/* test.bar2.inner */
#define MGOS_CONFIG_HAVE_TEST_BAR2_INNER
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR2_INNER
const struct mgos_config_bar_inner *mgos_config_get_test_bar2_inner(const struct mgos_config *cfg);
static inline const struct mgos_config_bar_inner *mgos_sys_config_get_test_bar2_inner(void) { return mgos_config_get_test_bar2_inner(&mgos_sys_config); }

/* test.bar2.inner.param2 */
#define MGOS_CONFIG_HAVE_TEST_BAR2_INNER_PARAM2
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR2_INNER_PARAM2
const char * mgos_config_get_test_bar2_inner_param2(const struct mgos_config *cfg);
const char * mgos_config_get_default_test_bar2_inner_param2(void);
static inline const char * mgos_sys_config_get_test_bar2_inner_param2(void) { return mgos_config_get_test_bar2_inner_param2(&mgos_sys_config); }
static inline const char * mgos_sys_config_get_default_test_bar2_inner_param2(void) { return mgos_config_get_default_test_bar2_inner_param2(); }
void mgos_config_set_test_bar2_inner_param2(struct mgos_config *cfg, const char * v);
static inline void mgos_sys_config_set_test_bar2_inner_param2(const char * v) { mgos_config_set_test_bar2_inner_param2(&mgos_sys_config, v); }

/* test.bar2.inner.param3 */
#define MGOS_CONFIG_HAVE_TEST_BAR2_INNER_PARAM3
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR2_INNER_PARAM3
int mgos_config_get_test_bar2_inner_param3(const struct mgos_config *cfg);
int mgos_config_get_default_test_bar2_inner_param3(void);
static inline int mgos_sys_config_get_test_bar2_inner_param3(void) { return mgos_config_get_test_bar2_inner_param3(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_test_bar2_inner_param3(void) { return mgos_config_get_default_test_bar2_inner_param3(); }
void mgos_config_set_test_bar2_inner_param3(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_test_bar2_inner_param3(int v) { mgos_config_set_test_bar2_inner_param3(&mgos_sys_config, v); }

/* test.bar2.baz */
#define MGOS_CONFIG_HAVE_TEST_BAR2_BAZ
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR2_BAZ
const struct mgos_config_baz *mgos_config_get_test_bar2_baz(const struct mgos_config *cfg);
static inline const struct mgos_config_baz *mgos_sys_config_get_test_bar2_baz(void) { return mgos_config_get_test_bar2_baz(&mgos_sys_config); }

/* test.bar2.baz.bazaar */
#define MGOS_CONFIG_HAVE_TEST_BAR2_BAZ_BAZAAR
#define MGOS_SYS_CONFIG_HAVE_TEST_BAR2_BAZ_BAZAAR
int mgos_config_get_test_bar2_baz_bazaar(const struct mgos_config *cfg);
int mgos_config_get_default_test_bar2_baz_bazaar(void);
static inline int mgos_sys_config_get_test_bar2_baz_bazaar(void) { return mgos_config_get_test_bar2_baz_bazaar(&mgos_sys_config); }
static inline int mgos_sys_config_get_default_test_bar2_baz_bazaar(void) { return mgos_config_get_default_test_bar2_baz_bazaar(); }
void mgos_config_set_test_bar2_baz_bazaar(struct mgos_config *cfg, int v);
static inline void mgos_sys_config_set_test_bar2_baz_bazaar(int v) { mgos_config_set_test_bar2_baz_bazaar(&mgos_sys_config, v); }

bool mgos_sys_config_get(const struct mg_str key, struct mg_str *value);
bool mgos_sys_config_set(const struct mg_str key, const struct mg_str value, bool free_strings);

bool mgos_config_is_default_str(const char *s);

/* Backward compatibility. */
const struct mgos_conf_entry *mgos_config_schema(void);

#ifdef __cplusplus
}
#endif
