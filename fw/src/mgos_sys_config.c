/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#include "fw/src/mgos_sys_config.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/cs_file.h"
#include "common/json_utils.h"
#include "common/str_util.h"
#include "fw/src/mgos_atca.h"
#include "fw/src/mgos_config.h"
#include "fw/src/mgos_debug.h"
#include "fw/src/mgos_debug_hal.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_updater_common.h"
#include "fw/src/mgos_utils.h"
#include "fw/src/mgos_wifi.h"

#define MGOS_F_RELOAD_CONFIG MG_F_USER_5
#define PLACEHOLDER_CHAR '?'

#define CONF_USER_FILE "conf9.json"
#define CONF_USER_FILE_NUM_IDX 4

/* FOr backward compatibility */
#define CONF_USER_FILE_OLD "conf.json"
#define CONF_VENDOR_FILE "conf_vendor.json"

/* Must be provided externally, usually auto-generated. */
extern const char *build_id;
extern const char *build_timestamp;
extern const char *build_version;

bool s_initialized = false;
struct sys_config s_cfg;
struct sys_config *get_cfg(void) {
  return (s_initialized ? &s_cfg : NULL);
}

struct sys_ro_vars s_ro_vars;
const struct sys_ro_vars *get_ro_vars(void) {
  return &s_ro_vars;
}

static mgos_config_validator_fn *s_validators;
static int s_num_validators;

#if MG_ENABLE_FILESYSTEM
static struct mg_serve_http_opts s_http_server_opts;
#endif
static struct mg_connection *listen_conn;
static struct mg_connection *listen_conn_tun;

static int load_config_file(const char *filename, const char *acl,
                            struct sys_config *cfg);

void mgos_expand_mac_address_placeholders(char *str) {
  const char *mac = s_ro_vars.mac_address;
  int num_placeholders = 0;
  char *sp;
  if (mac == NULL) return;
  for (sp = str; sp != NULL && *sp != '\0'; sp++) {
    if (*sp == PLACEHOLDER_CHAR) num_placeholders++;
  }
  if (num_placeholders > 0 && num_placeholders <= 12 &&
      num_placeholders % 2 == 0 /* Allows use of single '?' w/o subst. */) {
    const char *msp = mac + 11; /* Start from the end */
    for (; sp >= str; sp--) {
      if (*sp == PLACEHOLDER_CHAR) *sp = *msp--;
    }
  }
}

static bool load_config_defaults(struct sys_config *cfg) {
  int i;
  char fname[sizeof(CONF_USER_FILE)];
  memset(cfg, 0, sizeof(*cfg));
  memcpy(fname, CONF_USER_FILE, sizeof(fname));
  const char *acl = "*";
  for (i = 0; i < MGOS_CONFIG_LEVEL_USER; i++) {
    fname[CONF_USER_FILE_NUM_IDX] = '0' + i;
    if (!load_config_file(fname, acl, cfg)) {
      /* conf0 must exist, everything else is optional. */
      if (i == 0) return false;
    }
    acl = cfg->conf_acl;
    /* Backward compat: load conf_vendor.json at level 5.5 */
    if (i == 5) {
      load_config_file(CONF_VENDOR_FILE, cfg->conf_acl, cfg);
      acl = cfg->conf_acl;
    }
  }
  return true;
}

bool save_cfg(const struct sys_config *cfg, char **msg) {
  bool result = false;
  struct sys_config defaults;
  memset(&defaults, 0, sizeof(defaults));
  *msg = NULL;
  int i;
  for (i = 0; i < s_num_validators; i++) {
    if (!s_validators[i](cfg, msg)) goto clean;
  }
  if (!load_config_defaults(&defaults)) {
    *msg = strdup("failed to load defaults");
    goto clean;
  }
  if (mgos_conf_emit_f(cfg, &defaults, sys_config_schema(), true /* pretty */,
                       CONF_USER_FILE)) {
    LOG(LL_INFO, ("Saved to %s", CONF_USER_FILE));
    result = true;
  } else {
    *msg = strdup("failed to write file");
  }
clean:
  mgos_conf_free(sys_config_schema(), &defaults);
  return result;
}

void mgos_config_reset(int level) {
  int i;
  char fname[sizeof(CONF_USER_FILE)];
  memcpy(fname, CONF_USER_FILE, sizeof(fname));
  for (i = MGOS_CONFIG_LEVEL_USER; i >= level && i > 0; i--) {
    fname[CONF_USER_FILE_NUM_IDX] = '0' + i;
    if (remove(fname) == 0) {
      LOG(LL_INFO, ("Removed %s", fname));
    }
  }
}

#if MGOS_ENABLE_WEB_CONFIG

#define JSON_HEADERS "Connection: close\r\nContent-Type: application/json"

static void send_cfg(const void *cfg, const struct mgos_conf_entry *schema,
                     struct http_message *hm, struct mg_connection *c) {
  mg_send_response_line(c, 200, JSON_HEADERS);
  mg_send(c, "\r\n", 2);
  bool pretty = (mg_vcmp(&hm->query_string, "pretty") == 0);
  mgos_conf_emit_cb(cfg, NULL, schema, pretty, &c->send_mbuf, NULL, NULL);
}

static void conf_handler(struct mg_connection *c, int ev, void *p,
                         void *user_data) {
  struct http_message *hm = (struct http_message *) p;
  if (ev != MG_EV_HTTP_REQUEST) return;
  LOG(LL_DEBUG, ("[%.*s] requested", (int) hm->uri.len, hm->uri.p));
  struct mbuf jsmb;
  struct json_out jsout = JSON_OUT_MBUF(&jsmb);
  mbuf_init(&jsmb, 0);
  char *msg = NULL;
  int status = -1;
  int rc = 200;
  if (mg_vcmp(&hm->uri, "/conf/defaults") == 0) {
    struct sys_config cfg;
    if (load_config_defaults(&cfg)) {
      send_cfg(&cfg, sys_config_schema(), hm, c);
      mgos_conf_free(sys_config_schema(), &cfg);
      status = 0;
    }
  } else if (mg_vcmp(&hm->uri, "/conf/current") == 0) {
    send_cfg(&s_cfg, sys_config_schema(), hm, c);
    status = 0;
  } else if (mg_vcmp(&hm->uri, "/conf/save") == 0) {
    struct sys_config tmp;
    memset(&tmp, 0, sizeof(tmp));
    if (load_config_defaults(&tmp)) {
      char *acl_copy = (tmp.conf_acl == NULL ? NULL : strdup(tmp.conf_acl));
      if (mgos_conf_parse(hm->body, acl_copy, sys_config_schema(), &tmp)) {
        status = (save_cfg(&tmp, &msg) ? 0 : -10);
      } else {
        status = -11;
      }
      free(acl_copy);
    } else {
      status = -10;
    }
    mgos_conf_free(sys_config_schema(), &tmp);
    if (status == 0) c->flags |= MGOS_F_RELOAD_CONFIG;
  } else if (mg_vcmp(&hm->uri, "/conf/reset") == 0) {
    struct stat st;
    if (stat(CONF_USER_FILE, &st) == 0) {
      status = remove(CONF_USER_FILE);
    } else {
      status = 0;
    }
    if (status == 0) c->flags |= MGOS_F_RELOAD_CONFIG;
  }

  if (status != 0) {
    json_printf(&jsout, "{status: %d", status);
    if (msg != NULL) {
      json_printf(&jsout, ", message: %Q}", msg);
    } else {
      json_printf(&jsout, "}");
    }
    LOG(LL_ERROR, ("Error: %.*s", (int) jsmb.len, jsmb.buf));
    rc = 500;
  }

  if (jsmb.len > 0) {
    mg_send_head(c, rc, jsmb.len, JSON_HEADERS);
    mg_send(c, jsmb.buf, jsmb.len);
  }
  c->flags |= MG_F_SEND_AND_CLOSE;
  mbuf_free(&jsmb);
  free(msg);
  (void) user_data;
}

static void reboot_handler(struct mg_connection *c, int ev, void *p,
                           void *user_data) {
  (void) p;
  if (ev != MG_EV_HTTP_REQUEST) return;
  LOG(LL_DEBUG, ("Reboot requested"));
  mg_send_head(c, 200, 0, JSON_HEADERS);
  c->flags |= (MG_F_SEND_AND_CLOSE | MGOS_F_RELOAD_CONFIG);
  (void) user_data;
}

static void ro_vars_handler(struct mg_connection *c, int ev, void *p,
                            void *user_data) {
  if (ev != MG_EV_HTTP_REQUEST) return;
  LOG(LL_DEBUG, ("RO-vars requested"));
  struct http_message *hm = (struct http_message *) p;
  send_cfg(&s_ro_vars, sys_ro_vars_schema(), hm, c);
  c->flags |= MG_F_SEND_AND_CLOSE;
  (void) user_data;
}
#endif /* MGOS_ENABLE_WEB_CONFIG */

#if MGOS_ENABLE_FILE_UPLOAD
static struct mg_str upload_fname(struct mg_connection *nc,
                                  struct mg_str fname) {
  struct mg_str res = {NULL, 0};
  (void) nc;
  if (mgos_conf_check_access(fname, get_cfg()->http.upload_acl)) {
    res = fname;
  }
  return res;
}

static void upload_handler(struct mg_connection *c, int ev, void *p,
                           void *user_data) {
  mg_file_upload_handler(c, ev, p, upload_fname, user_data);
}
#endif

static void mgos_http_ev(struct mg_connection *c, int ev, void *p,
                         void *user_data) {
  switch (ev) {
    case MG_EV_ACCEPT: {
      char addr[32];
      mg_sock_addr_to_str(&c->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      LOG(LL_INFO, ("%p HTTP connection from %s", c, addr));
      break;
    }
    case MG_EV_HTTP_REQUEST: {
#if MG_ENABLE_FILESYSTEM
      struct http_message *hm = (struct http_message *) p;
      LOG(LL_INFO, ("%p %.*s %.*s", c, (int) hm->method.len, hm->method.p,
                    (int) hm->uri.len, hm->uri.p));

      mg_serve_http(c, p, s_http_server_opts);
/*
 * NOTE: `mg_serve_http()` manages closing connection when appropriate,
 * so, we should not set `MG_F_SEND_AND_CLOSE` here
 */
#else
      mg_http_send_error(c, 404, "Not Found");
#endif
      break;
    }
    case MG_EV_HTTP_MULTIPART_REQUEST: {
      mg_http_send_error(c, 404, "Not Found");
      break;
    }
    case MG_EV_CLOSE: {
      /* If we've sent the reply to the server, and should reboot, reboot */
      if (c->flags & MGOS_F_RELOAD_CONFIG) {
        c->flags &= ~MGOS_F_RELOAD_CONFIG;
        mgos_system_restart(0);
      }
      break;
    }
  }
  (void) user_data;
}

#if MGOS_ENABLE_WIFI && MGOS_ENABLE_TUNNEL
static void on_wifi_ready(enum mgos_wifi_status event, void *arg) {
  if (listen_conn_tun != NULL) {
    /* Depending on the WiFi status, allow or disallow tunnel reconnection */
    switch (event) {
      case MGOS_WIFI_DISCONNECTED:
        listen_conn_tun->flags |= MG_F_TUN_DO_NOT_RECONNECT;
        break;
      case MGOS_WIFI_IP_ACQUIRED:
        listen_conn_tun->flags &= ~MG_F_TUN_DO_NOT_RECONNECT;
        break;
      default:
        break;
    }
  }

  (void) arg;
}
#endif /* MGOS_ENABLE_WIFI */

enum mgos_init_result mgos_sys_config_init_http(
    const struct sys_config_http *cfg,
    const struct sys_config_device *device_cfg) {
  (void) device_cfg;

  if (!cfg->enable) {
    return MGOS_INIT_OK;
  }

  if (cfg->listen_addr == NULL) {
    LOG(LL_WARN, ("HTTP Server disabled, listening address is empty"));
    return MGOS_INIT_OK; /* At this moment it is just warning */
  }

#if MG_ENABLE_FILESYSTEM
  s_http_server_opts.hidden_file_pattern = cfg->hidden_files;
  s_http_server_opts.auth_domain = cfg->auth_domain;
  s_http_server_opts.global_auth_file = cfg->auth_file;
#endif

  struct mg_bind_opts opts;
  memset(&opts, 0, sizeof(opts));
#if MG_ENABLE_SSL
  opts.ssl_cert = cfg->ssl_cert;
  opts.ssl_key = cfg->ssl_key;
  opts.ssl_ca_cert = cfg->ssl_ca_cert;
#if CS_PLATFORM == CS_P_ESP8266
/*
 * ESP8266 cannot handle DH of any kind, unless there's hardware acceleration,
 * it's too slow.
 */
#if MGOS_ENABLE_ATCA
  if (mbedtls_atca_is_available()) {
    opts.ssl_cipher_suites =
        "TLS-ECDHE-ECDSA-WITH-AES-128-GCM-SHA256:"
        "TLS-ECDHE-RSA-WITH-AES-128-GCM-SHA256:"
        "TLS-ECDH-ECDSA-WITH-AES-128-GCM-SHA256:"
        "TLS-ECDH-ECDSA-WITH-AES-128-CBC-SHA256:"
        "TLS-ECDH-ECDSA-WITH-AES-128-CBC-SHA:"
        "TLS-ECDH-RSA-WITH-AES-128-GCM-SHA256:"
        "TLS-ECDH-RSA-WITH-AES-128-CBC-SHA256:"
        "TLS-ECDH-RSA-WITH-AES-128-CBC-SHA:"
        "TLS-RSA-WITH-AES-128-GCM-SHA256:"
        "TLS-RSA-WITH-AES-128-CBC-SHA256:"
        "TLS-RSA-WITH-AES-128-CBC-SHA";
  } else
#endif /* MGOS_ENABLE_ATCA */
    opts.ssl_cipher_suites =
        "TLS-RSA-WITH-AES-128-GCM-SHA256:"
        "TLS-RSA-WITH-AES-128-CBC-SHA256:"
        "TLS-RSA-WITH-AES-128-CBC-SHA";
#endif /* CS_PLATFORM == CS_P_ESP8266 */
#endif /* MG_ENABLE_SSL */
  listen_conn =
      mg_bind_opt(mgos_get_mgr(), cfg->listen_addr, mgos_http_ev, NULL, opts);

  if (!listen_conn) {
    LOG(LL_ERROR, ("Error binding to [%s]", cfg->listen_addr));
    return MGOS_INIT_CONFIG_WEB_SERVER_LISTEN_FAILED;
  }

  mg_set_protocol_http_websocket(listen_conn);
  LOG(LL_INFO, ("HTTP server started on [%s]%s", cfg->listen_addr,
#if MG_ENABLE_SSL
                (opts.ssl_cert ? " (SSL)" : "")
#else
                ""
#endif
                    ));

#if MGOS_ENABLE_TUNNEL
  if (cfg->tunnel.enable && device_cfg->id != NULL &&
      device_cfg->password != NULL) {
    char *tun_addr = NULL;
    /*
     * NOTE: we won't free `tun_addr`, because when reconnect happens, this
     * address string will be accessed again.
     */
    if (mg_asprintf(&tun_addr, 0, "ws://%s:%s@%s.%s", device_cfg->id,
                    device_cfg->password, device_cfg->id,
                    cfg->tunnel.addr) < 0) {
      return MGOS_INIT_OUT_OF_MEMORY;
    }
    listen_conn_tun = mg_bind_opt(mgos_get_mgr(), tun_addr, mgos_http_ev, opts);

    if (listen_conn_tun == NULL) {
      LOG(LL_ERROR, ("Error binding to [%s]", tun_addr));
      return MGOS_INIT_CONFIG_WEB_SERVER_LISTEN_FAILED;
    } else {
#if MGOS_ENABLE_WIFI
      /*
       * Wifi is not yet ready, so we need to set a flag which prevents the
       * tunnel from reconnecting. The flag will be cleared when wifi connection
       * is ready.
       */
      listen_conn_tun->flags |= MG_F_TUN_DO_NOT_RECONNECT;
      mgos_wifi_add_on_change_cb(on_wifi_ready, NULL);
#endif
    }

    mg_set_protocol_http_websocket(listen_conn_tun);
    LOG(LL_INFO, ("Tunneled HTTP server started on [%s]%s", tun_addr,
#if MG_ENABLE_SSL
                  (opts.ssl_cert ? " (SSL)" : "")
#else
                  ""
#endif
                      ));
  }
#endif

#if MGOS_ENABLE_WEB_CONFIG
  mgos_register_http_endpoint("/conf/", conf_handler, NULL);
  mgos_register_http_endpoint("/reboot", reboot_handler, NULL);
  mgos_register_http_endpoint("/ro_vars", ro_vars_handler, NULL);
#endif
#if MGOS_ENABLE_FILE_UPLOAD
  mgos_register_http_endpoint("/upload", upload_handler, NULL);
#endif

  return MGOS_INIT_OK;
}

static int load_config_file(const char *filename, const char *acl,
                            struct sys_config *cfg) {
  char *data = NULL, *acl_copy = NULL;
  size_t size;
  int result = 1;
  LOG(LL_DEBUG, ("=== Loading %s", filename));
  data = cs_read_file(filename, &size);
  if (data == NULL) {
    /* File not found or read error */
    result = 0;
    goto clean;
  }
  /* Make a temporary copy, in case it gets overridden while loading. */
  acl_copy = (acl != NULL ? strdup(acl) : NULL);
  if (!mgos_conf_parse(mg_mk_str_n(data, size), acl_copy, sys_config_schema(),
                       cfg)) {
    LOG(LL_ERROR, ("Failed to parse %s", filename));
    result = 0;
    goto clean;
  }
clean:
  free(data);
  free(acl_copy);
  return result;
}

void mbedtls_debug_set_threshold(int threshold);

enum mgos_init_result mgos_sys_config_init(void) {
  /* Load system defaults - mandatory */
  if (!load_config_defaults(&s_cfg)) {
    LOG(LL_ERROR, ("Failed to load config defaults"));
    return MGOS_INIT_CONFIG_LOAD_DEFAULTS_FAILED;
  }

  /*
   * Check factory reset GPIO. We intentionally do it before loading
   * CONF_USER_FILE
   * so that it cannot be overridden by the end user.
   */
  if (s_cfg.debug.factory_reset_gpio >= 0) {
    int gpio = s_cfg.debug.factory_reset_gpio;
    mgos_gpio_set_mode(gpio, MGOS_GPIO_MODE_INPUT);
    mgos_gpio_set_pull(gpio, MGOS_GPIO_PULL_UP);
    if (mgos_gpio_read(gpio) == 0) {
      LOG(LL_WARN, ("Factory reset requested via GPIO%d", gpio));
      if (remove(CONF_USER_FILE) == 0) {
        LOG(LL_WARN, ("Removed %s", CONF_USER_FILE));
      }
      /* Continue as if nothing happened, no reboot necessary. */
    }
  }

#if CS_PLATFORM != CS_P_PIC32
  struct stat st;
  if (stat(CONF_USER_FILE_OLD, &st) == 0) {
    rename(CONF_USER_FILE_OLD, CONF_USER_FILE);
  }
#endif
  /* Successfully loaded system config. Try overrides - they are optional. */
  load_config_file(CONF_USER_FILE, s_cfg.conf_acl, &s_cfg);

  s_initialized = true;

  if (mgos_set_stdout_uart(s_cfg.debug.stdout_uart) != MGOS_INIT_OK) {
    return MGOS_INIT_CONFIG_INVALID_STDOUT_UART;
  }
#ifdef MGOS_DEBUG_UART
  /*
   * This is likely to be the last message on the old console. Inform the user
   * about what's about to happen, otherwise the user may be confused why the
   * output suddenly stopped. Happened to me (rojer). More than once, in fact.
   */
  if (s_cfg.debug.stderr_uart != MGOS_DEBUG_UART) {
    LOG(LL_INFO, ("Switching debug to UART%d", s_cfg.debug.stderr_uart));
  }
#endif
  if (mgos_set_stderr_uart(s_cfg.debug.stderr_uart) != MGOS_INIT_OK) {
    return MGOS_INIT_CONFIG_INVALID_STDERR_UART;
  }
#if MGOS_ENABLE_DEBUG_UDP
  if (s_cfg.debug.udp_log_addr != NULL) {
    LOG(LL_INFO, ("Sending logs to UDP %s", s_cfg.debug.udp_log_addr));
    if (mgos_debug_udp_init(s_cfg.debug.udp_log_addr) != MGOS_INIT_OK) {
      return MGOS_INIT_DEBUG_INIT_FAILED;
    }
  }
#endif /* MGOS_ENABLE_DEBUG_UDP */
  if (s_cfg.debug.level > _LL_MIN && s_cfg.debug.level < _LL_MAX) {
    cs_log_set_level((enum cs_log_level) s_cfg.debug.level);
  }
#if MG_SSL_IF == MG_SSL_IF_MBEDTLS
  mbedtls_debug_set_threshold(s_cfg.debug.mbedtls_level);
#endif

  s_ro_vars.arch = CS_STRINGIFY_MACRO(FW_ARCHITECTURE);
  s_ro_vars.fw_id = build_id;
  s_ro_vars.fw_timestamp = build_timestamp;
  s_ro_vars.fw_version = build_version;

  /* Init mac address readonly var - users may use it as device ID */
  uint8_t mac[6];
  device_get_mac_address(mac);
  if (mg_asprintf((char **) &s_ro_vars.mac_address, 0,
                  "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3],
                  mac[4], mac[5]) < 0) {
    return MGOS_INIT_OUT_OF_MEMORY;
  }
  LOG(LL_INFO, ("MAC: %s", s_ro_vars.mac_address));
  mgos_expand_mac_address_placeholders(s_cfg.device.id);
#if MGOS_ENABLE_MQTT
  mgos_expand_mac_address_placeholders(s_cfg.debug.stdout_topic);
  mgos_expand_mac_address_placeholders(s_cfg.debug.stderr_topic);
#endif

  LOG(LL_INFO, ("WDT: %d seconds", s_cfg.sys.wdt_timeout));
  mgos_wdt_set_timeout(s_cfg.sys.wdt_timeout);
  mgos_wdt_set_feed_on_poll(true);

  mgos_get_mgr()->hexdump_file = s_cfg.debug.mg_mgr_hexdump_file;

  return MGOS_INIT_OK;
}

void mgos_register_config_validator(mgos_config_validator_fn fn) {
  s_validators = (mgos_config_validator_fn *) realloc(
      s_validators, (s_num_validators + 1) * sizeof(*s_validators));
  if (s_validators == NULL) return;
  s_validators[s_num_validators++] = fn;
}

void mgos_register_http_endpoint_opt(const char *uri_path,
                                     mg_event_handler_t handler,
                                     struct mg_http_endpoint_opts opts) {
  if (listen_conn != NULL) {
    mg_register_http_endpoint_opt(listen_conn, uri_path, handler, opts);
  }
  if (listen_conn_tun != NULL) {
    mg_register_http_endpoint_opt(listen_conn_tun, uri_path, handler, opts);
  }
}

void mgos_register_http_endpoint(const char *uri_path,
                                 mg_event_handler_t handler, void *user_data) {
  struct mg_http_endpoint_opts opts;
  memset(&opts, 0, sizeof(opts));
  opts.user_data = user_data;
  opts.auth_domain = get_cfg()->http.auth_domain;
  opts.auth_file = get_cfg()->http.auth_file;
  mgos_register_http_endpoint_opt(uri_path, handler, opts);
}

struct mg_connection *mgos_get_sys_http_server(void) {
  return listen_conn;
}
