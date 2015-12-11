#include <ets_sys.h>
#include <osapi.h>
#include <user_interface.h>
#include <stdio.h>

#include "config.h"
#include "cs_file.h"
#include "cs_dbg.h"
#include "esp_uart.h"
#include "mongoose.h"
#include "sj_mongoose.h"

#define MG_F_RELOAD_CONFIG MG_F_USER_5
#define PLACEHOLDER_CHAR '?'

#ifndef FW_VERSION
#define FW_VERSION "esp866_" __DATE__
#endif

struct ro_var *g_ro_vars = NULL;

static struct mg_serve_http_opts s_http_server_opts;
static const char *s_fw_version = FW_VERSION;
static const char *s_architecture = "esp8266";
static char s_mac_address[13];

static int do_station(const struct sys_config *cfg) {
  struct station_config sta_cfg;

  wifi_station_disconnect();

  if (strlen(cfg->wifi.sta.ssid) > sizeof(sta_cfg.ssid) ||
      strlen(cfg->wifi.sta.pass) > sizeof(sta_cfg.password)) {
    LOG(LL_ERROR, ("WIFI station SSID or PASS too long"));
    return 0;
  }

  memset(&sta_cfg, 0, sizeof(sta_cfg));
  strncpy((char *) sta_cfg.ssid, cfg->wifi.sta.ssid, sizeof(sta_cfg.ssid));
  strncpy((char *) sta_cfg.password, cfg->wifi.sta.pass,
          sizeof(sta_cfg.password));
  sta_cfg.bssid_set = 0;

  wifi_station_set_config_current(&sta_cfg);

  LOG(LL_DEBUG, ("Joining %s", sta_cfg.ssid));
  wifi_station_connect();

  return 1;
}

static void expand_placeholders(char *str) {
  int num_placeholders = 0;
  char *sp;
  for (sp = str; *sp != '\0'; sp++) {
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

static int do_ap(const struct sys_config *cfg) {
  struct softap_config ap_cfg;
  struct ip_info info;
  struct dhcps_lease dhcps;

  memset(&ap_cfg, 0, sizeof(ap_cfg));

  int pass_len = strlen(cfg->wifi.ap.pass);
  int ssid_len = strlen(cfg->wifi.ap.ssid);

  if (ssid_len > sizeof(ap_cfg.ssid) || pass_len > sizeof(ap_cfg.password)) {
    LOG(LL_ERROR, ("AP SSID or PASS too long"));
    return 0;
  }

  if (pass_len != 0 && pass_len < 8) {
    /*
     * If we don't check pwd len here and it will be less than 8 chars
     * esp will setup _open_ wifi with name ESP_<mac address here>
     */
    LOG(LL_ERROR, ("AP password too short"));
    return 0;
  }

  strncpy((char *) ap_cfg.ssid, cfg->wifi.ap.ssid, sizeof(ap_cfg.ssid));
  strncpy((char *) ap_cfg.password, cfg->wifi.ap.pass, sizeof(ap_cfg.password));
  ap_cfg.ssid_len = ssid_len;
  if (pass_len != 0) {
    ap_cfg.authmode = AUTH_WPA2_PSK;
  }
  ap_cfg.channel = cfg->wifi.ap.channel;
  ap_cfg.ssid_hidden = (cfg->wifi.ap.hidden != 0);
  ap_cfg.max_connection = 1;
  ap_cfg.beacon_interval = 100; /* ms */

  /* Expand ??s, if any, to MAC address. */
  expand_placeholders((char *) ap_cfg.ssid);

  LOG(LL_DEBUG, ("Setting up %s on channel %d", ap_cfg.ssid, ap_cfg.channel));
  wifi_softap_set_config_current(&ap_cfg);

  LOG(LL_DEBUG, ("Restarting DHCP server"));
  wifi_softap_dhcps_stop();

  /*
   * We have to set ESP's IP address explicitly also, GW IP has to be the
   * same. Using ap_dhcp_start as IP address for ESP
   */
  info.netmask.addr = ipaddr_addr(cfg->wifi.ap.dhcp_netmask);
  info.ip.addr = info.gw.addr = ipaddr_addr(cfg->wifi.ap.dhcp_start);
  wifi_set_ip_info(SOFTAP_IF, &info);

  dhcps.start_ip.addr = ipaddr_addr(cfg->wifi.ap.dhcp_start);
  dhcps.end_ip.addr = ipaddr_addr(cfg->wifi.ap.dhcp_end);
  wifi_softap_set_dhcps_lease(&dhcps);

  wifi_softap_dhcps_start();

  wifi_get_ip_info(SOFTAP_IF, &info);
  LOG(LL_INFO, ("AP address is " IPSTR "", IP2STR(&info.ip)));

  return 1;
}

static int do_wifi(const struct sys_config *cfg) {
  wifi_set_opmode_current(STATION_MODE);
  wifi_station_set_auto_connect(0);
  wifi_station_disconnect();

  if (cfg->wifi.ap.enable && cfg->wifi.sta.enable) {
    wifi_set_opmode_current(STATIONAP_MODE);
  } else if (cfg->wifi.ap.enable) {
    wifi_set_opmode_current(SOFTAP_MODE);
  } else if (cfg->wifi.sta.enable) {
    wifi_set_opmode_current(STATION_MODE);
  } else {
    LOG(LL_ERROR, ("No wifi mode specified"));
    return 0;
  }

  if (cfg->wifi.sta.enable && !do_station(cfg)) {
    LOG(LL_ERROR, ("Failed to initialize WiFi connection"));
  }

  if (cfg->wifi.ap.enable && !do_ap(cfg)) {
    LOG(LL_ERROR, ("Failed to setup AP"));
  }

  /*
   * TODO(alashkin, lsm): what should return this function if STA is OK
   * but AP failed and vice versa?
   */

  return 1;
}

int load_config(const char *defaults_file, const char *overrides_file,
                struct sys_config *cfg) {
  char *defaults = NULL, *overrides = NULL;
  size_t size;
  int result = 0;

  /* Load system defaults - mandatory */
  memset(cfg, 0, sizeof(*cfg));
  if ((defaults = cs_read_file(defaults_file, &size)) != NULL &&
      parse_sys_config(defaults, cfg, 1)) {
    /* Successfully loaded system config. Try overrides - they are optional. */
    if (overrides_file != NULL) {
      overrides = cs_read_file(overrides_file, &size);
      parse_sys_config(overrides, cfg, 0);
    }
    result = 1;
  }
  free(defaults);
  free(overrides);

  return result;
}

static void mongoose_ev_handler(struct mg_connection *c, int ev, void *p) {
  const char *json_headers =
      "Connection: close\r\nContent-Type: application/json";

  LOG(LL_VERBOSE_DEBUG,
      ("%s: %p ev %d, fl %lx l %lu %lu, heap %u", __func__, p, ev, c->flags,
       (unsigned long) c->recv_mbuf.len, (unsigned long) c->send_mbuf.len,
       system_get_free_heap_size()));

  switch (ev) {
    case MG_EV_HTTP_REQUEST: {
      struct http_message *hm = (struct http_message *) p;
      char *buf = NULL;

      LOG(LL_DEBUG,
          ("HTTP request to [%.*s]\n", (int) hm->message.len, hm->message.p));

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
      LOG(LL_DEBUG, ("mbuf len: %lu\n", (unsigned long) c->send_mbuf.len));
      c->flags |= MG_F_SEND_AND_CLOSE;
      free(buf);
      break;
    }
    case MG_EV_CLOSE:
      /* If we've sent the reply to the server, and should reboot, reboot */
      if (c->flags & MG_F_RELOAD_CONFIG) {
        c->flags &= ~MG_F_RELOAD_CONFIG;
        system_restart();
      }
      break;
  }
}

int apply_config(const struct sys_config *cfg) {
  REGISTER_RO_VAR(fw_version, &s_fw_version);
  REGISTER_RO_VAR(arch, &s_architecture);

  /* Initialize debug first */
  uart_debug_init(0, 0);
  uart_redirect_debug(cfg->debug.mode);
  cs_log_set_level(cfg->debug.level);

  /* Init mac address readonly var - users may use it as device ID */
  uint8_t mac[6];
  wifi_get_macaddr(SOFTAP_IF, mac);
  snprintf(s_mac_address, sizeof(s_mac_address), "%02X%02X%02X%02X%02X%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  static const char *mac_address_ptr = s_mac_address;
  REGISTER_RO_VAR(mac_address, &mac_address_ptr);
  LOG(LL_INFO, ("MAC: %s\n", s_mac_address));

  if (!do_wifi(cfg)) {
    return 0;
  }

  if (cfg->http.enable) {
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
  }

  return 1;
}
