/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "common/cs_file.h"
#include "smartjs/src/sj_mongoose.h"
#include "smartjs/src/device_config.h"
#include "smartjs/src/sj_config.h"
#include "smartjs/src/sj_gpio.h"

#define MG_F_RELOAD_CONFIG MG_F_USER_5
#define PLACEHOLDER_CHAR '?'

#ifndef FW_ARCHITECTURE
#define FW_ARCHITECTURE "UNKNOWN_ARCH"
#endif

/*
 * The only sys_config instance
 * If application requires access to its configuration
 * in run-time (not in init-time, it can access it through
 * const struct sys_config *get_cfg()
 */
struct sys_config s_cfg;
struct sys_config *get_cfg() {
  return &s_cfg;
}

/* Global vars */
struct ro_var *g_ro_vars = NULL;

/* Must be provided externally, usually auto-generated. */
extern const char *build_id;
extern const char *build_timestamp;
extern const char *build_version;

static const char *s_architecture = FW_ARCHITECTURE;
static struct mg_serve_http_opts s_http_server_opts;
static char s_mac_address[13];
static const char *mac_address_ptr = s_mac_address;
static struct mg_connection *listen_conn;

static int load_config_file(const char *filename, const char *acl, int required,
                            struct sys_config *cfg);

static void export_read_only_vars_to_v7(struct v7 *v7) {
  struct ro_var *rv;
  if (v7 == NULL) return;
  v7_val_t obj = v7_mk_object(v7);
  for (rv = g_ro_vars; rv != NULL; rv = rv->next) {
    v7_set(v7, obj, rv->name, ~0, v7_mk_string(v7, *rv->ptr, ~0, 1));
  }
  v7_val_t Sys = v7_get(v7, v7_get_global(v7), "Sys", ~0);
  v7_set(v7, Sys, "ro_vars", ~0, obj);
}

void expand_mac_address_placeholders(char *str) {
  int num_placeholders = 0;
  char *sp;
  for (sp = str; sp != NULL && *sp != '\0'; sp++) {
    if (*sp == PLACEHOLDER_CHAR) num_placeholders++;
  }
  if (num_placeholders > 0 && num_placeholders < 12 &&
      num_placeholders % 2 == 0 /* Allows use of single '?' w/o subst. */) {
    char *msp = s_mac_address + 11;
    for (; sp >= str; sp--) {
      if (*sp == PLACEHOLDER_CHAR) *sp = *msp--;
    }
  }
}

static int load_config_defaults(struct sys_config *cfg) {
  /* TODO(rojer): Figure out what to do about merging two different defaults. */
  if (!load_config_file(CONF_SYS_DEFAULTS_FILE, "*", 0, cfg)) return 0;
  if (!load_config_file(CONF_APP_DEFAULTS_FILE, cfg->conf_acl, 0, cfg))
    return 0;
  /* Vendor config is optional. */
  load_config_file(CONF_VENDOR_FILE, cfg->conf_acl, 0, cfg);
  return 1;
}

#define JSON_HEADERS "Connection: close\r\nContent-Type: application/json"

static int save_json(const struct mg_str *data, const char *file_name) {
  FILE *fp;
  int len = parse_json(data->p, data->len, NULL, 0);
  if (len <= 0) {
    LOG(LL_ERROR, ("%s\n", "Invalid JSON string"));
    return 0;
  }
  fp = fopen("tmp", "w");
  if (fp == NULL) {
    LOG(LL_ERROR, ("Error opening file for writing\n"));
    return 0;
  }
  if (fwrite(data->p, 1, len, fp) != (size_t) len) {
    LOG(LL_ERROR, ("Error writing file\n"));
    fclose(fp);
    return 0;
  }
  if (fclose(fp) != 0) {
    LOG(LL_ERROR, ("Error closing file\n"));
    return 0;
  }
  if (rename("tmp", file_name) != 0) {
    LOG(LL_ERROR, ("Error renaming file to %s\n", file_name));
    return 0;
  }
  return 1;
}

static void conf_handler(struct mg_connection *c, int ev, void *p) {
  struct http_message *hm = (struct http_message *) p;
  if (ev != MG_EV_HTTP_REQUEST) return;
  LOG(LL_DEBUG, ("[%.*s] requested", (int) hm->uri.len, hm->uri.p));
  char *json = NULL;
  int status = -1;
  int rc = 200;
  if (mg_vcmp(&hm->uri, "/conf/defaults") == 0) {
    struct sys_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    if (load_config_defaults(&cfg)) {
      json = emit_sys_config(&cfg);
    }
  } else if (mg_vcmp(&hm->uri, "/conf/current") == 0) {
    json = emit_sys_config(&s_cfg);
  } else if (mg_vcmp(&hm->uri, "/conf/save") == 0) {
    status = (save_json(&hm->body, CONF_FILE) != 1);
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

  if (json == NULL) {
    if (asprintf(&json, "{\"status\": %d}\n", status) < 0) {
      json = "{\"status\": -1}";
    } else {
      rc = (status == 0 ? 200 : 500);
    }
  }

  {
    int len = strlen(json);
    mg_send_head(c, rc, len, JSON_HEADERS);
    mg_send(c, json, len);
    free(json);
  }
  c->flags |= MG_F_SEND_AND_CLOSE;
}

static void reboot_handler(struct mg_connection *c, int ev, void *p) {
  (void) p;
  if (ev != MG_EV_HTTP_REQUEST) return;
  LOG(LL_DEBUG, ("Reboot requested"));
  mg_send_head(c, 200, 0, JSON_HEADERS);
  c->flags |= (MG_F_SEND_AND_CLOSE | MG_F_RELOAD_CONFIG);
}

static void ro_vars_handler(struct mg_connection *c, int ev, void *p) {
  (void) p;
  if (ev != MG_EV_HTTP_REQUEST) return;
  LOG(LL_DEBUG, ("RO-vars requested"));
  /* Reply with JSON object that contains read-only variables */
  mg_send_head(c, 200, -1, JSON_HEADERS);
  mg_printf_http_chunk(c, "{");
  struct ro_var *rv;
  for (rv = g_ro_vars; rv != NULL; rv = rv->next) {
    mg_printf_http_chunk(c, "%s\n  \"%s\": \"%s\"", rv == g_ro_vars ? "" : ",",
                         rv->name, *rv->ptr);
  }
  mg_printf_http_chunk(c, "\n}\n");
  mg_printf_http_chunk(c, ""); /* Zero chunk - end of response */
  c->flags |= MG_F_SEND_AND_CLOSE;
}

struct mg_str upload_fname(struct mg_connection *nc, struct mg_str fname) {
  struct mg_str res = {NULL, 0};
  if (sj_conf_check_access(fname, get_cfg()->http.upload_acl)) {
    res = fname;
  }
  return res;
  (void) nc;
}

static void upload_handler(struct mg_connection *c, int ev, void *p) {
  mg_file_upload_handler(c, ev, p, upload_fname);
}

static void mongoose_ev_handler(struct mg_connection *c, int ev, void *p) {
  LOG(LL_VERBOSE_DEBUG,
      ("%p ev %d p %p fl %lx l %lu %lu", c, ev, p, c->flags,
       (unsigned long) c->recv_mbuf.len, (unsigned long) c->send_mbuf.len));

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
        device_reboot();
      }
      break;
    }
  }
}

void device_register_http_endpoint(const char *uri,
                                   mg_event_handler_t handler) {
  if (listen_conn != NULL) {
    mg_register_http_endpoint(listen_conn, uri, handler);
  }
}

static int init_web_server(const struct sys_config *cfg) {
  /*
   * Usually, we start to connect/listen in
   * EVENT_STAMODE_GOT_IP/EVENT_SOFTAPMODE_STACONNECTED  handlers
   * The only obvious reason for this is to specify IP address
   * in `mg_bind` function. But it is not clear, for what we have to
   * provide IP address in case of ESP
   */
  if (cfg->http.enable_webdav) {
    s_http_server_opts.dav_document_root = ".";
  }
  if (cfg->http.hidden_files) {
    s_http_server_opts.hidden_file_pattern = strdup(cfg->http.hidden_files);
  }

  listen_conn = mg_bind(&sj_mgr, cfg->http.listen_addr, mongoose_ev_handler);
  if (!listen_conn) {
    LOG(LL_ERROR, ("Error binding to [%s]", cfg->http.listen_addr));
    return 0;
  } else {
    mg_register_http_endpoint(listen_conn, "/conf/", conf_handler);
    mg_register_http_endpoint(listen_conn, "/reboot", reboot_handler);
    mg_register_http_endpoint(listen_conn, "/ro_vars", ro_vars_handler);
    mg_register_http_endpoint(listen_conn, "/upload", upload_handler);

    mg_set_protocol_http_websocket(listen_conn);
    LOG(LL_INFO, ("HTTP server started on [%s]", cfg->http.listen_addr));
  }
  return 1;
}

static int load_config_file(const char *filename, const char *acl, int required,
                            struct sys_config *cfg) {
  char *data, *acl_copy;
  size_t size;
  int result = 1;
  LOG(LL_DEBUG, ("=== Loading %s", filename));
  data = cs_read_file(filename, &size);
  /* Make a temporary copy, in case it gets overridden while loading. */
  acl_copy = (acl != NULL ? strdup(acl) : NULL);
  if (data == NULL || !parse_sys_config(data, acl_copy, required, cfg)) {
    LOG(required ? LL_ERROR : LL_INFO, ("Failed to load %s", filename));
    result = 0;
  }
  free(data);
  free(acl_copy);
  return result;
}

int init_device(struct v7 *v7) {
  int result = 1;
  uint8_t mac[6] = "";

  /* Load system defaults - mandatory */
  memset(&s_cfg, 0, sizeof(s_cfg));
  if (!load_config_defaults(&s_cfg)) {
    LOG(LL_ERROR, ("Failed to load config defaults"));
    return 0;
  }

#ifndef SJ_DISABLE_GPIO
  /*
   * Check factory reset GPIO. We intentionally do it before loading CONF_FILE
   * so that it cannot be overridden by the end user.
   */
  if (s_cfg.debug.factory_reset_gpio >= 0) {
    int gpio = s_cfg.debug.factory_reset_gpio;
    sj_gpio_set_mode(gpio, GPIO_MODE_INPUT, GPIO_PULL_PULLUP);
    if (sj_gpio_read(gpio) == GPIO_LEVEL_LOW) {
      LOG(LL_WARN, ("Factory reset requested via GPIO%d", gpio));
      if (remove(CONF_FILE) == 0) {
        LOG(LL_WARN, ("Removed %s", CONF_FILE));
      }
      /* Continue as if nothing happened, no reboot necessary. */
    }
  }
#endif

  /* Successfully loaded system config. Try overrides - they are optional. */
  load_config_file(CONF_FILE, s_cfg.conf_acl, 0, &s_cfg);

  REGISTER_RO_VAR(fw_id, &build_id);
  REGISTER_RO_VAR(fw_timestamp, &build_timestamp);
  REGISTER_RO_VAR(fw_version, &build_version);
  REGISTER_RO_VAR(arch, &s_architecture);

  /* Init mac address readonly var - users may use it as device ID */
  device_get_mac_address(mac);
  snprintf(s_mac_address, sizeof(s_mac_address), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  REGISTER_RO_VAR(mac_address, &mac_address_ptr);
  LOG(LL_INFO, ("MAC: %s", s_mac_address));

  if (get_cfg()->wifi.ap.ssid != NULL) {
    expand_mac_address_placeholders((char *) get_cfg()->wifi.ap.ssid);
  }

  result = device_init_platform(v7, get_cfg());
  if (result != 0) {
    if (get_cfg()->http.enable) {
      result = init_web_server(get_cfg());
    }
  } else {
    LOG(LL_ERROR, ("Platform init failed"));
  }

  /* NOTE(lsm): must be done last */
  export_read_only_vars_to_v7(v7);

  return result;
}

int update_sysconf(struct v7 *v7, const char *path, v7_val_t val) {
  v7_val_t sys = v7_get(v7, v7_get_global(v7), "Sys", ~0);
  if (!v7_is_object(sys)) {
    return 1;
  }

  v7_val_t conf = v7_get(v7, sys, "conf", ~0);
  if (!v7_is_object(conf)) {
    return 1;
  }

  v7_val_t prev_obj, curr_obj;
  prev_obj = curr_obj = conf;

  const char *prev_tok, *curr_tok;
  prev_tok = curr_tok = path;

  for (;;) {
    while (*curr_tok != 0 && *curr_tok != '.') {
      curr_tok++;
    }
    curr_obj = v7_get(v7, prev_obj, prev_tok, (curr_tok - prev_tok));
    if (v7_is_undefined(curr_obj)) {
      return 1;
    } else if (!v7_is_object(curr_obj)) {
      v7_set(v7, prev_obj, prev_tok, (curr_tok - prev_tok), val);
      return 0;
    }
    if (*curr_tok == 0) {
      return 1;
    }
    curr_tok++;
    prev_tok = curr_tok;
    prev_obj = curr_obj;
  }

  return 1;
}
