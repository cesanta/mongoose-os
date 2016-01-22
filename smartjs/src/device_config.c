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

/*
 * Check if both conf_sys_defaults.clubby.device_id and conf.clubby.device_id
 * are empty and generate new device_id and device_psk
 * device_id = //api.cesanta.com/u/esp8266/[esp mac address]
 * device_psk = [8 random hex numbers]
 * NOTE: device_id and device_psk will be stored in conf.json,
 * not in conf_sys_defaults.json, to keep these values during
 * OTA update
 */
void init_config(struct v7 *v7) {
  v7_val_t confv;

  /* Looking conf_sys_defaults.json for device_id */
  enum v7_err res = v7_parse_json_file(v7, SYSTEM_DEFAULT_JSON_FILE, &confv);
  if (res != V7_OK) {
    LOG(LL_ERROR, ("Cannot load %s", SYSTEM_DEFAULT_JSON_FILE));
    return;
  }

  v7_val_t clubbyv = v7_get(v7, confv, "clubby", ~0);
  if (v7_is_undefined(clubbyv)) {
    LOG(LL_ERROR, ("Clubby configuration is not found"));
    return;
  }

  v7_val_t device_id_v = v7_get(v7, clubbyv, "device_id", ~0);

  if (v7_is_string(device_id_v) &&
      strlen(v7_to_cstring(v7, &device_id_v)) != 0) {
    /* Ok, we have device_id in conf_sys_defaults.json */
    return;
  }

  /* Trying conf.json */
  res = v7_parse_json_file(v7, OVERRIDES_JSON_FILE, &confv);
  if (res != V7_OK) {
    confv = v7_mk_object(v7);
  }

  clubbyv = v7_get(v7, confv, "clubby", ~0);

  if (!v7_is_object(clubbyv)) {
    clubbyv = v7_mk_object(v7);
    v7_set(v7, confv, "clubby", ~0, clubbyv);
  }

  device_id_v = v7_get(v7, clubbyv, "device_id", ~0);

  if (v7_is_string(device_id_v) &&
      strlen(v7_to_cstring(v7, &device_id_v)) != 0) {
    /* Ok, we have device_id in conf.json */
    return;
  }

  /* Creating device_id */
  uint8_t mac[6];
  char mac_str[13];
  device_get_mac_address(mac);
  snprintf(mac_str, sizeof(mac_str), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1],
           mac[2], mac[3], mac[4], mac[5]);
  v7_def(v7, clubbyv, "device_id", ~0, 0, v7_mk_string(v7, mac_str, ~0, 1));

  /* Creating device_psk */
  char device_psk[8];
  int i;
  for (i = 0; i < 8; i += 2) {
    sprintf(device_psk + i, "%02X", (int) ((double) rand() / RAND_MAX * 255));
  }
  v7_def(v7, clubbyv, "device_psk", ~0, 0, v7_mk_string(v7, device_psk, 8, 1));

  /* Saving updated conf.json */
  char json_buf[100];
  char *p =
      v7_stringify(v7, confv, json_buf, sizeof(json_buf), V7_STRINGIFY_JSON);

  LOG(LL_DEBUG, ("Config updated:\n%s", p));

  FILE *conf = fopen("conf.json", "w");
  if (conf == NULL) {
    LOG(LL_ERROR, ("Cannot open conf.json"));
  } else {
    fwrite(p, 1, strlen(p), conf);
    fclose(conf);
  }

  if (p != json_buf) {
    free(p);
  }
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

  if (get_cfg()->wifi.ap.ssid != NULL) {
    expand_mac_address_placeholders((char *) get_cfg()->wifi.ap.ssid);
  }

  if (result && (result = device_init_platform(get_cfg())) != 0 &&
      get_cfg()->http.enable) {
    result = init_web_server(get_cfg());
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
