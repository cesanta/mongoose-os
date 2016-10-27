/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */
#include "fw/src/mg_sys_config.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common/cs_file.h"
#include "common/json_utils.h"
#include "fw/src/mg_config.h"
#include "fw/src/mg_gpio.h"
#include "fw/src/mg_hal.h"
#include "fw/src/mg_init.h"
#include "fw/src/mg_mongoose.h"
#include "fw/src/mg_updater_common.h"
#include "fw/src/mg_utils.h"

#define MG_F_RELOAD_CONFIG MG_F_USER_5
#define PLACEHOLDER_CHAR '?'

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

static mg_config_validator_fn *s_validators;
static int s_num_validators;

static struct mg_serve_http_opts s_http_server_opts;
static struct mg_connection *listen_conn;

static int load_config_file(const char *filename, const char *acl,
                            struct sys_config *cfg);

void mg_expand_mac_address_placeholders(char *str) {
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
  memset(cfg, 0, sizeof(*cfg));
  if (!load_config_file(CONF_DEFAULTS_FILE, "*", cfg)) {
    return false;
  }
  /* Vendor config is optional. */
  load_config_file(CONF_VENDOR_FILE, cfg->conf_acl, cfg);
  return true;
}

bool save_cfg(const struct sys_config *cfg, char **msg) {
  bool result = false;
  struct sys_config defaults;
  memset(&defaults, 0, sizeof(defaults));
  *msg = NULL;
  for (int i = 0; i < s_num_validators; i++) {
    if (!s_validators[i](cfg, msg)) goto clean;
  }
  if (!load_config_defaults(&defaults)) {
    *msg = strdup("failed to load defaults");
    goto clean;
  }
  if (mg_conf_emit_f(cfg, &defaults, sys_config_schema(), true /* pretty */,
                     CONF_FILE)) {
    LOG(LL_INFO, ("Saved to %s", CONF_FILE));
    result = true;
  } else {
    *msg = "failed to write file";
  }
clean:
  mg_conf_free(sys_config_schema(), &defaults);
  return result;
}

#if MG_ENABLE_WEB_CONFIG

#define JSON_HEADERS "Connection: close\r\nContent-Type: application/json"

static void send_cfg(const void *cfg, const struct mg_conf_entry *schema,
                     struct http_message *hm, struct mg_connection *c) {
  mg_send_response_line(c, 200, JSON_HEADERS);
  mg_send(c, "\r\n", 2);
  bool pretty = (mg_vcmp(&hm->query_string, "pretty") == 0);
  mg_conf_emit_cb(cfg, NULL, schema, pretty, &c->send_mbuf, NULL, NULL);
}

static void conf_handler(struct mg_connection *c, int ev, void *p) {
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
      mg_conf_free(sys_config_schema(), &cfg);
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
      if (mg_conf_parse(hm->body, acl_copy, sys_config_schema(), &tmp)) {
        status = (save_cfg(&tmp, &msg) ? 0 : -10);
      } else {
        status = -11;
      }
      free(acl_copy);
    } else {
      status = -10;
    }
    mg_conf_free(sys_config_schema(), &tmp);
    if (status == 0) c->flags |= MG_F_RELOAD_CONFIG;
  } else if (mg_vcmp(&hm->uri, "/conf/reset") == 0) {
    struct stat st;
    if (stat(CONF_FILE, &st) == 0) {
      status = remove(CONF_FILE);
    } else {
      status = 0;
    }
    if (status == 0) c->flags |= MG_F_RELOAD_CONFIG;
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
}

static void reboot_handler(struct mg_connection *c, int ev, void *p) {
  (void) p;
  if (ev != MG_EV_HTTP_REQUEST) return;
  LOG(LL_DEBUG, ("Reboot requested"));
  mg_send_head(c, 200, 0, JSON_HEADERS);
  c->flags |= (MG_F_SEND_AND_CLOSE | MG_F_RELOAD_CONFIG);
}

static void ro_vars_handler(struct mg_connection *c, int ev, void *p) {
  if (ev != MG_EV_HTTP_REQUEST) return;
  LOG(LL_DEBUG, ("RO-vars requested"));
  struct http_message *hm = (struct http_message *) p;
  send_cfg(&s_ro_vars, sys_ro_vars_schema(), hm, c);
  c->flags |= MG_F_SEND_AND_CLOSE;
}
#endif /* MG_ENABLE_WEB_CONFIG */

#if MG_ENABLE_FILE_UPLOAD
static struct mg_str upload_fname(struct mg_connection *nc,
                                  struct mg_str fname) {
  struct mg_str res = {NULL, 0};
  (void) nc;
  if (mg_conf_check_access(fname, get_cfg()->http.upload_acl)) {
    res = fname;
  }
  return res;
}

static void upload_handler(struct mg_connection *c, int ev, void *p) {
  mg_file_upload_handler(c, ev, p, upload_fname);
}
#endif

#if MG_ENABLE_UPDATER_POST || MG_ENABLE_UPDATER_CLUBBY
static void update_action_handler(struct mg_connection *c, int ev, void *p) {
  if (ev != MG_EV_HTTP_REQUEST) return;
  struct http_message *hm = (struct http_message *) p;
  bool is_commit = (mg_vcmp(&hm->uri, "/update/commit") == 0);
  bool ok = (is_commit ? mg_upd_commit() : mg_upd_revert(false /* reboot */));
  mg_send_response_line(c, (ok ? 200 : 400),
                        "Content-Type: text/html\r\n"
                        "Connection: close");
  mg_printf(c, "\r\n%s\r\n", (ok ? "Ok" : "Error"));
  c->flags |= MG_F_SEND_AND_CLOSE;
  if (!is_commit) mg_system_restart_after(100);
}
#endif

static void mongoose_ev_handler(struct mg_connection *c, int ev, void *p) {
  switch (ev) {
    case MG_EV_ACCEPT: {
      char addr[32];
      mg_sock_addr_to_str(&c->sa, addr, sizeof(addr),
                          MG_SOCK_STRINGIFY_IP | MG_SOCK_STRINGIFY_PORT);
      LOG(LL_INFO, ("%p HTTP connection from %s", c, addr));
      break;
    }
    case MG_EV_HTTP_REQUEST: {
      struct http_message *hm = (struct http_message *) p;
      LOG(LL_INFO, ("%p %.*s %.*s", c, (int) hm->method.len, hm->method.p,
                    (int) hm->uri.len, hm->uri.p));

      mg_serve_http(c, p, s_http_server_opts);
      c->flags |= MG_F_SEND_AND_CLOSE;
      break;
    }
    case MG_EV_CLOSE: {
      /* If we've sent the reply to the server, and should reboot, reboot */
      if (c->flags & MG_F_RELOAD_CONFIG) {
        c->flags &= ~MG_F_RELOAD_CONFIG;
        mg_system_restart(0);
      }
      break;
    }
  }
}

enum mg_init_result mg_sys_config_init_http(const struct sys_config_http *cfg) {
  /*
   * Usually, we start to connect/listen in
   * EVENT_STAMODE_GOT_IP/EVENT_SOFTAPMODE_STACONNECTED  handlers
   * The only obvious reason for this is to specify IP address
   * in `mg_bind` function. But it is not clear, for what we have to
   * provide IP address in case of ESP
   */
  if (cfg->hidden_files) {
    s_http_server_opts.hidden_file_pattern = strdup(cfg->hidden_files);
    if (s_http_server_opts.hidden_file_pattern == NULL) {
      return MG_INIT_OUT_OF_MEMORY;
    }
  }

  listen_conn = mg_bind(mg_get_mgr(), cfg->listen_addr, mongoose_ev_handler);
  if (!listen_conn) {
    LOG(LL_ERROR, ("Error binding to [%s]", cfg->listen_addr));
    return MG_INIT_CONFIG_WEB_SERVER_LISTEN_FAILED;
  } else {
#if MG_ENABLE_WEB_CONFIG
    mg_register_http_endpoint(listen_conn, "/conf/", conf_handler);
    mg_register_http_endpoint(listen_conn, "/reboot", reboot_handler);
    mg_register_http_endpoint(listen_conn, "/ro_vars", ro_vars_handler);
#endif
#if MG_ENABLE_FILE_UPLOAD
    mg_register_http_endpoint(listen_conn, "/upload", upload_handler);
#endif
#if MG_ENABLE_UPDATER_POST || MG_ENABLE_UPDATER_CLUBBY
    mg_register_http_endpoint(listen_conn, "/update/commit",
                              update_action_handler);
    mg_register_http_endpoint(listen_conn, "/update/revert",
                              update_action_handler);
#endif

    mg_set_protocol_http_websocket(listen_conn);
    LOG(LL_INFO, ("HTTP server started on [%s]", cfg->listen_addr));
  }

  return MG_INIT_OK;
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
  if (!mg_conf_parse(mg_mk_str(data), acl_copy, sys_config_schema(), cfg)) {
    LOG(LL_ERROR, ("Failed to parse %s", filename));
    result = 0;
    goto clean;
  }
clean:
  free(data);
  free(acl_copy);
  return result;
}

enum mg_init_result mg_sys_config_init(void) {
  /* Load system defaults - mandatory */
  if (!load_config_defaults(&s_cfg)) {
    LOG(LL_ERROR, ("Failed to load config defaults"));
    return MG_INIT_CONFIG_LOAD_DEFAULTS_FAILED;
  }

#if MG_ENABLE_GPIO_API
  /*
   * Check factory reset GPIO. We intentionally do it before loading CONF_FILE
   * so that it cannot be overridden by the end user.
   */
  if (s_cfg.debug.factory_reset_gpio >= 0) {
    int gpio = s_cfg.debug.factory_reset_gpio;
    mg_gpio_set_mode(gpio, GPIO_MODE_INPUT, GPIO_PULL_PULLUP);
    if (mg_gpio_read(gpio) == GPIO_LEVEL_LOW) {
      LOG(LL_WARN, ("Factory reset requested via GPIO%d", gpio));
      if (remove(CONF_FILE) == 0) {
        LOG(LL_WARN, ("Removed %s", CONF_FILE));
      }
      /* Continue as if nothing happened, no reboot necessary. */
    }
  }
#endif

  /* Successfully loaded system config. Try overrides - they are optional. */
  load_config_file(CONF_FILE, s_cfg.conf_acl, &s_cfg);

  if (s_cfg.debug.level > _LL_MIN && s_cfg.debug.level < _LL_MAX) {
    cs_log_set_level((enum cs_log_level) s_cfg.debug.level);
  }

  s_ro_vars.arch = FW_ARCHITECTURE;
  s_ro_vars.fw_id = build_id;
  s_ro_vars.fw_timestamp = build_timestamp;
  s_ro_vars.fw_version = build_version;

  /* Init mac address readonly var - users may use it as device ID */
  uint8_t mac[6];
  device_get_mac_address(mac);
  if (asprintf((char **) &s_ro_vars.mac_address, "%02X%02X%02X%02X%02X%02X",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]) < 0) {
    return MG_INIT_OUT_OF_MEMORY;
  }
  LOG(LL_INFO, ("MAC: %s", s_ro_vars.mac_address));

  s_initialized = true;

  return MG_INIT_OK;
}

void mg_register_config_validator(mg_config_validator_fn fn) {
  s_validators = (mg_config_validator_fn *) realloc(
      s_validators, (s_num_validators + 1) * sizeof(*s_validators));
  if (s_validators == NULL) return;
  s_validators[s_num_validators++] = fn;
}

struct mg_connection *mg_get_http_listening_conn(void) {
  return listen_conn;
}
