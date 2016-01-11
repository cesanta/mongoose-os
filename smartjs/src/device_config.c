#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#include "mongoose/mongoose.h"
#include "common/cs_file.h"
#include "smartjs/src/sj_mongoose.h"
#include "smartjs/src/device_config.h"

#define SYSTEM_DEFAULT_JSON_FILE "conf_sys_defaults.json"

#define MG_F_RELOAD_CONFIG MG_F_USER_5
#define PLACEHOLDER_CHAR '?'

#ifndef FW_VERSION
#define FW_VERSION "UNKNOWN_FW_VERSION"
#endif

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

static const char *s_fw_version = FW_VERSION;
static const char *s_architecture = FW_ARCHITECTURE;
static struct mg_serve_http_opts s_http_server_opts;
static char s_mac_address[13];
static const char *mac_address_ptr = s_mac_address;

static void export_read_only_vars_to_v7(struct v7 *v7) {
  struct ro_var *rv;
  if (v7 == NULL) return;
  v7_val_t obj = v7_create_object(v7);
  for (rv = g_ro_vars; rv != NULL; rv = rv->next) {
    v7_set(v7, obj, rv->name, ~0, v7_create_string(v7, *rv->ptr, ~0, 1));
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

static void mongoose_ev_handler(struct mg_connection *c, int ev, void *p) {
  const char *json_headers =
      "Connection: close\r\nContent-Type: application/json";

  LOG(LL_VERBOSE_DEBUG,
      ("%s: %p ev %d, fl %lx l %lu %lu", __func__, p, ev, c->flags,
       (unsigned long) c->recv_mbuf.len, (unsigned long) c->send_mbuf.len));

  switch (ev) {
    case MG_EV_HTTP_REQUEST: {
      struct http_message *hm = (struct http_message *) p;
      char *buf = NULL;

      if (mg_vcmp(&hm->uri, "/reboot") == 0) {
        c->flags |= MG_F_RELOAD_CONFIG;
        mg_send_head(c, 200, 0, json_headers);
      } else if (mg_vcmp(&hm->uri, "/ro_vars") == 0) {
        /* Reply with JSON object that contains read-only variables */
        mg_send_head(c, 200, -1, json_headers);
        mg_printf_http_chunk(c, "{");
        struct ro_var *rv;
        for (rv = g_ro_vars; rv != NULL; rv = rv->next) {
          mg_printf_http_chunk(c, "%s\n  \"%s\": \"%s\"",
                               rv == g_ro_vars ? "" : ",", rv->name, *rv->ptr);
        }
        mg_printf_http_chunk(c, "\n}\n");
        mg_printf_http_chunk(c, ""); /* Zero chunk - end of response */
      } else if (mg_vcmp(&hm->uri, "/factory_reset") == 0) {
        remove("conf.json");
        c->flags |= MG_F_RELOAD_CONFIG;
        mg_send_head(c, 200, 0, json_headers);
      } else {
        mg_serve_http(c, p, s_http_server_opts);
      }

      LOG(LL_DEBUG,
          ("[%.*s] -> [%.*s]\n", (int) ((hm->body.p - 1) - hm->message.p),
           hm->message.p, (int) c->send_mbuf.len, c->send_mbuf.buf));

      c->flags |= MG_F_SEND_AND_CLOSE;
      free(buf);
      break;
    }
    case MG_EV_CLOSE:
      /* If we've sent the reply to the server, and should reboot, reboot */
      if (c->flags & MG_F_RELOAD_CONFIG) {
        c->flags &= ~MG_F_RELOAD_CONFIG;
        device_reboot();
      }
      break;
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

  struct mg_connection *conn;
  conn = mg_bind(&sj_mgr, cfg->http.port, mongoose_ev_handler);
  if (!conn) {
    LOG(LL_ERROR, ("Error binding to port [%s]", cfg->http.port));
    return 0;
  } else {
    mg_set_protocol_http_websocket(conn);
    LOG(LL_INFO, ("HTTP server started on port [%s]", cfg->http.port));
  }
  return 1;
}

int init_device(struct v7 *v7) {
  char *defaults = NULL, *overrides = NULL;
  size_t size;
  int result = 0;
  uint8_t mac[6] = "";

  /* Load system defaults - mandatory */
  memset(&s_cfg, 0, sizeof(s_cfg));
  if ((defaults = cs_read_file(SYSTEM_DEFAULT_JSON_FILE, &size)) != NULL &&
      parse_sys_config(defaults, &s_cfg, 1)) {
    /* Successfully loaded system config. Try overrides - they are optional. */
    overrides = cs_read_file(OVERRIDES_JSON_FILE, &size);
    parse_sys_config(overrides, &s_cfg, 0);
    result = 1;
  }
  free(defaults);
  free(overrides);

  REGISTER_RO_VAR(fw_version, &s_fw_version);
  REGISTER_RO_VAR(arch, &s_architecture);

  /* Init mac address readonly var - users may use it as device ID */
  device_get_mac_address(mac);
  snprintf(s_mac_address, sizeof(s_mac_address), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  REGISTER_RO_VAR(mac_address, &mac_address_ptr);
  LOG(LL_INFO, ("MAC: %s\n", s_mac_address));

  expand_mac_address_placeholders((char *) get_cfg()->wifi.ap.ssid);

  if (result && (result = device_init_platform(get_cfg())) != 0 &&
      get_cfg()->http.enable) {
    result = init_web_server(get_cfg());
  }

  /* NOTE(lsm): must be done last */
  export_read_only_vars_to_v7(v7);

  return result;
}
