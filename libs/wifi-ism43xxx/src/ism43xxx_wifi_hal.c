/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* WiFi HAL API impl */

#include "mgos.h"
#include "mgos_timers.h"
#include "mgos_wifi_hal.h"

#include "ism43xxx_core.h"

#define ISM43XXX_LINE_SEP "\r\n"

static struct ism43xxx_ctx *s_ctx = NULL;
static mgos_timer_id s_poll_timer_id = MGOS_INVALID_TIMER_ID;
static bool s_poll_in_progress = false;

static void ism43xxx_wifi_poll_timer_cb(void *arg);

char *asp(const char *fmt, ...) {
  char *res = NULL;
  va_list ap;
  va_start(ap, fmt);
  mg_avprintf(&res, 0, fmt, ap);
  va_end(ap);
  return res;
}

/* AP */

bool ism43xxx_parse_mac(const char *s, uint8_t mac[6]) {
  unsigned int m0, m1, m2, m3, m4, m5;
  if (sscanf(s, "%02X:%02X:%02X:%02X:%02X:%02X", &m0, &m1, &m2, &m3, &m4,
             &m5) != 6) {
    return false;
  }
  mac[0] = m0;
  mac[1] = m1;
  mac[2] = m2;
  mac[3] = m3;
  mac[4] = m4;
  mac[5] = m5;
  return true;
}

static bool ism43xxx_ar_cb(struct ism43xxx_ctx *c,
                           const struct ism43xxx_cmd *cmd, bool ok,
                           const struct mg_str payload) {
  int n = 0;
  struct ism43xxx_client_info ap_clients[ISM43XXX_AP_MAX_CLIENTS];
  const struct mg_str sep = mg_mk_str(ISM43XXX_LINE_SEP);
  if (!ok) return false;
  /* Mark clients that are still connected. */
  int gen = 0;
  for (int i = 0; i < (int) ARRAY_SIZE(c->ap_clients); i++) {
    gen = MAX(gen, c->ap_clients[i].gen);
  }
  if (++gen == 0) ++gen;
  const char *eol;
  struct mg_str buf = payload;
  while ((eol = mg_strstr(buf, sep)) != NULL && n < ISM43XXX_AP_MAX_CLIENTS) {
    struct mg_str s;
    struct mg_str e = mg_mk_str_n(buf.p, eol - buf.p);
    struct ism43xxx_client_info *ci = &ap_clients[n];
    buf.p += e.len + sep.len;
    buf.len -= e.len + sep.len;
    e = mg_next_comma_list_entry_n(e, &s, NULL); /* Index, skip */
    e = mg_next_comma_list_entry_n(e, &s, NULL); /* MAC */
    if (s.p == NULL || !ism43xxx_parse_mac(s.p, ci->mac)) continue;
    e = mg_next_comma_list_entry_n(e, &s, NULL); /* MAC */
    if (s.p == NULL) continue;
    ci->rssi = strtol(s.p, NULL, 10);
    if (ci->rssi >= 0) continue;
    ci->gen = gen;
    for (int i = 0; i < (int) ARRAY_SIZE(c->ap_clients); i++) {
      if (memcmp(c->ap_clients[i].mac, ap_clients[n].mac,
                 sizeof(ap_clients[n].mac)) == 0) {
        c->ap_clients[i].rssi = ci->rssi;
        c->ap_clients[i].gen = gen;
        ci->gen = 0;
        break;
      }
    }
    if (ci->gen != 0) n++;
  }
  /* Sweep away clients that are gone. */
  struct mgos_wifi_dev_event_info dei = {
      .ev = MGOS_WIFI_EV_AP_STA_DISCONNECTED,
  };
  for (int i = 0; i < (int) ARRAY_SIZE(c->ap_clients); i++) {
    struct ism43xxx_client_info *ci = &c->ap_clients[i];
    if (ci->gen == 0 || ci->gen == gen) continue;
    memcpy(dei.ap_sta_disconnected.mac, ci->mac,
           sizeof(dei.ap_sta_disconnected.mac));
    memset(&c->ap_clients[i], 0, sizeof(c->ap_clients[i]));
    mgos_wifi_dev_event_cb(&dei);
  }
  /* Add new clients */
  dei.ev = MGOS_WIFI_EV_AP_STA_CONNECTED;
  for (int i = 0; i < n; i++) {
    struct ism43xxx_client_info *ci = &ap_clients[i];
    for (int j = 0; i < (int) ARRAY_SIZE(c->ap_clients); j++) {
      if (c->ap_clients[j].gen == 0) {
        memcpy(dei.ap_sta_connected.mac, ci->mac,
               sizeof(dei.ap_sta_disconnected.mac));
        memcpy(&c->ap_clients[j], ci, sizeof(c->ap_clients[j]));
        mgos_wifi_dev_event_cb(&dei);
        break;
      }
    }
  }
  (void) cmd;
  return ok;
}

static bool ism43xxx_pre_ad_cb(struct ism43xxx_ctx *c,
                               const struct ism43xxx_cmd *cmd, bool ok,
                               struct mg_str p) {
  LOG(LL_INFO, ("AP starting..."));
  (void) c;
  (void) cmd;
  (void) p;
  return ok;
}

static bool ism43xxx_ad_cb(struct ism43xxx_ctx *c,
                           const struct ism43xxx_cmd *cmd, bool ok,
                           struct mg_str payload) {
  c->ap_running = ok;
  if (ok) {
    LOG(LL_INFO, ("AP started"));
  } else {
    LOG(LL_INFO, ("AP failed to start: %.*s", (int) payload.len, payload.p));
  }
  (void) c;
  (void) cmd;
  (void) payload;
  return ok;
}

bool mgos_wifi_dev_ap_setup(const struct mgos_config_wifi_ap *cfg) {
  bool res = false;
  struct ism43xxx_ctx *c = s_ctx;

  if (!cfg->enable) {
    if (c->mode == ISM43XXX_MODE_AP) {
      ism43xxx_reset(s_ctx, true /* hold */);
      mgos_clear_timer(s_poll_timer_id);
      s_poll_timer_id = MGOS_INVALID_TIMER_ID;
    }
    res = true;
    goto out;
  }

  int max_clients = MIN(cfg->max_connections, ISM43XXX_AP_MAX_CLIENTS);

  const struct ism43xxx_cmd ism43xxx_ap_setup_seq[] = {
      /* Disable STA and AP, if enabled, to start with a clean slate. */
      {.cmd = "CD", .ph = ism43xxx_ignore_error},
      {.cmd = "AE", .ph = ism43xxx_ignore_error},
      /* SSID */
      {.cmd = asp("AS=0,%s", cfg->ssid), .free = true},
      /* Security (0 = open, 3 = WPA2, 4 = WPA+WPA2) */
      {.cmd = (cfg->pass ? "A1=4" : "A1=0")},
      /* Password */
      {.cmd = asp("A2=%s", (cfg->pass ? cfg->pass : "")), .free = true},
      /* Channel; 0 = auto */
      {.cmd = asp("AC=%d", cfg->channel), .free = true},
      /* Max num clients */
      {.cmd = asp("AT=%d", max_clients), .free = true},
      /* AP's IP address. Netmask is not configurable. */
      {.cmd = asp("Z6=%s", cfg->ip), .free = true},
      /* No power saving */
      {.cmd = "ZP=0", .ph = ism43xxx_pre_ad_cb},
      /* Activate */
      {.cmd = "AD", .ph = ism43xxx_ad_cb, .timeout = 10},
      {.cmd = NULL},
  };

  if (c->phase == ISM43XXX_PHASE_CMD && c->mode == ISM43XXX_MODE_IDLE) {
    res = ism43xxx_send_seq(c, ism43xxx_ap_setup_seq, true /* copy */) != NULL;
  } else {
    ism43xxx_reset(c, false /* hold */);
    res = ism43xxx_send_seq(c, ism43xxx_ap_setup_seq, true /* copy */) != NULL;
  }

  if (res) {
    c->mode = ISM43XXX_MODE_AP;
    mgos_clear_timer(s_poll_timer_id);
    s_poll_timer_id = mgos_set_timer(1000, MGOS_TIMER_REPEAT,
                                     ism43xxx_wifi_poll_timer_cb, NULL);
    s_poll_in_progress = false;
  }

out:
  return res;
}

/* STA */

void ism43xxx_set_sta_status(struct ism43xxx_ctx *c, bool connected,
                             bool force) {
  if (connected != c->sta_connected || force) {
    c->sta_connected = connected;
    if (connected) {
      struct mgos_wifi_dev_event_info dei = {
          .ev = MGOS_WIFI_EV_STA_CONNECTED,
          // BSSID and channel are not available.
      };
      mgos_wifi_dev_event_cb(&dei);
      if (c->sta_ip_info.ip.sin_addr.s_addr != 0) {
        dei.ev = MGOS_WIFI_EV_STA_IP_ACQUIRED;
        mgos_wifi_dev_event_cb(&dei);
      }
    } else {
      struct mgos_wifi_dev_event_info dei = {
          .ev = MGOS_WIFI_EV_STA_DISCONNECTED,
          .sta_disconnected =
              {
               .reason = 0,  // Reason? We don't know.
              },
      };
      mgos_wifi_dev_event_cb(&dei);
      if (c->if_disconnect_cb != NULL) c->if_disconnect_cb(c->if_cb_arg);
    }
  }
}

static bool ism43xxx_pre_c0_cb(struct ism43xxx_ctx *c,
                               const struct ism43xxx_cmd *cmd, bool ok,
                               struct mg_str p) {
  LOG(LL_INFO, ("STA connecting..."));
  (void) c;
  (void) cmd;
  (void) ok;
  (void) p;
  return true;
}

static bool ism43xxx_cr_cb(struct ism43xxx_ctx *c,
                           const struct ism43xxx_cmd *cmd, bool ok,
                           struct mg_str p) {
  c->sta_rssi = (ok ? strtol(p.p, NULL, 10) : 0);
  (void) cmd;
  return true;
}

static bool ism43xxx_cinfo_cb(struct ism43xxx_ctx *c,
                              const struct ism43xxx_cmd *cmd, bool ok,
                              struct mg_str p) {
  struct mg_str s = mg_strstrip(p), ip, nm, gw, dns, status, unused;
  bool sta_connected = false;
  if (ok) {
    memset(&c->sta_ip_info, 0, sizeof(c->sta_ip_info));
    s = mg_next_comma_list_entry_n(s, &unused, NULL); /* SSID */
    s = mg_next_comma_list_entry_n(s, &unused, NULL); /* Pass */
    s = mg_next_comma_list_entry_n(s, &unused, NULL); /* Security type */
    s = mg_next_comma_list_entry_n(s, &unused, NULL); /* DHCP on/off */
    s = mg_next_comma_list_entry_n(s, &unused, NULL); /* IP ver */
    s = mg_next_comma_list_entry_n(s, &ip, NULL);
    s = mg_next_comma_list_entry_n(s, &nm, NULL);
    s = mg_next_comma_list_entry_n(s, &gw, NULL);
    s = mg_next_comma_list_entry_n(s, &dns, NULL);
    s = mg_next_comma_list_entry_n(s, &unused, NULL); /* DNS2 */
    s = mg_next_comma_list_entry_n(s, &unused, NULL); /* Num retries */
    s = mg_next_comma_list_entry_n(s, &unused, NULL); /* Autoconnect */
    s = mg_next_comma_list_entry_n(s, &unused, NULL); /* Authentication (?) */
    s = mg_next_comma_list_entry_n(s, &unused, NULL); /* Country */
    s = mg_next_comma_list_entry_n(s, &status, NULL);
    sta_connected = (status.len == 1 && status.p[0] == '1');
    if (sta_connected) {
      if (ip.len > 0 && nm.len > 0 && gw.len > 0) {
        mgos_net_str_to_ip_n(ip, &c->sta_ip_info.ip);
        mgos_net_str_to_ip_n(nm, &c->sta_ip_info.netmask);
        mgos_net_str_to_ip_n(gw, &c->sta_ip_info.gw);
      }
      if (dns.len > 3) {
        /* FW versions prior to 3.5.2.4 do not set the DNS server field. */
        struct mg_str dnsp = mg_mk_str_n(dns.p, 3);
        if (mg_vcmp(&dnsp, "255") == 0) {
          LOG(LL_WARN, ("BUG: DNS is not set, using default. "
                        "Please Update the es-WiFi module FW."));
        } else {
        }
      }
    }
  }
  ism43xxx_set_sta_status(c, sta_connected, true /* force */);
  (void) cmd;
  return true;
}

static bool ism43xxx_cs_cb(struct ism43xxx_ctx *c,
                           const struct ism43xxx_cmd *cmd, bool ok,
                           struct mg_str p) {
  if (ok) {
    bool sta_connected = (p.p[0] == '1');
    ism43xxx_set_sta_status(c, sta_connected, false /* force */);
  }
  (void) cmd;
  return ok;
}

static bool ism43xxx_cd_cb(struct ism43xxx_ctx *c,
                           const struct ism43xxx_cmd *cmd, bool ok,
                           struct mg_str p) {
  if (ok) {
    ism43xxx_set_sta_status(c, false /* connected */, false /* force */);
  }
  (void) cmd;
  (void) p;
  return ok;
}

const struct ism43xxx_cmd ism43xxx_sta_connect_seq[] = {
    {.cmd = "C?", .ph = ism43xxx_pre_c0_cb},
    /* Suppress the error from C0 to prevent automatic retry.
     * CS will report disconnection and upper layer will re-connect. */
    {.cmd = "C0", .ph = ism43xxx_ignore_error, .timeout = 20},
    {.cmd = "CR", .ph = ism43xxx_cr_cb}, /* Get RSSI */
    {.cmd = "C?", .ph = ism43xxx_cinfo_cb},
    {.cmd = NULL},
};

const struct ism43xxx_cmd ism43xxx_sta_disconnect_seq[] = {
    {.cmd = "CD", .ph = ism43xxx_cd_cb}, {.cmd = NULL},
};

bool mgos_wifi_dev_sta_setup(const struct mgos_config_wifi_sta *cfg) {
  bool res = false;
  struct ism43xxx_ctx *c = s_ctx;

  if (!cfg->enable) {
    if (c->mode == ISM43XXX_MODE_STA) {
      ism43xxx_reset(s_ctx, true /* hold */);
      mgos_clear_timer(s_poll_timer_id);
      s_poll_timer_id = MGOS_INVALID_TIMER_ID;
    }
    res = true;
    goto out;
  }

  bool static_ip = (cfg->ip != NULL && cfg->netmask != NULL);

  const struct ism43xxx_cmd ism43xxx_sta_setup_seq[] = {
      /* Disable STA and AP, if enabled, to start with a clean slate. */
      {.cmd = "CD", .ph = ism43xxx_ignore_error},
      {.cmd = "AE", .ph = ism43xxx_ignore_error},
      {.cmd = asp("C1=%s", cfg->ssid), .free = true},
      {.cmd = asp("C2=%s", (cfg->pass ? cfg->pass : "")), .free = true},
      /* Security (0 = open, 4 = WPA+WPA2) */
      {.cmd = (cfg->pass != NULL ? "C3=4" : "C3=0")},
      /* DHCP (1 = on, 0 = off, use static IP) */
      {.cmd = (static_ip ? "C4=0" : "C4=1")},
      {.cmd = asp("C6=%s", (cfg->ip ? cfg->ip : "0.0.0.0")), .free = true},
      {.cmd = asp("C7=%s", (cfg->netmask ? cfg->netmask : "0.0.0.0")),
       .free = true},
      {.cmd = asp("C8=%s", (cfg->gw ? cfg->gw : "0.0.0.0")), .free = true},
      /* 0 = IPv4, 1 = IPv6 */
      {.cmd = "C5=0"},
      /* Retry count - try once. Reconnects will be handled by higher layers. */
      {.cmd = "CB=1"},
      {.cmd = NULL},
  };

  if (c->phase == ISM43XXX_PHASE_CMD && c->mode == ISM43XXX_MODE_IDLE) {
    res = ism43xxx_send_seq(c, ism43xxx_sta_setup_seq, true /* copy */) != NULL;
  } else {
    ism43xxx_reset(c, false /* hold */);
    res = ism43xxx_send_seq(c, ism43xxx_sta_setup_seq, true /* copy */) != NULL;
  }

  if (res) c->mode = ISM43XXX_MODE_STA;

out:
  return res;
}

bool mgos_wifi_dev_sta_connect(void) {
  struct ism43xxx_ctx *c = (struct ism43xxx_ctx *) s_ctx;
  if (c->mode != ISM43XXX_MODE_STA) return false;
  mgos_clear_timer(s_poll_timer_id);
  s_poll_timer_id = mgos_set_timer(1000, MGOS_TIMER_REPEAT,
                                   ism43xxx_wifi_poll_timer_cb, NULL);
  s_poll_in_progress = false;
  return (ism43xxx_send_seq(c, ism43xxx_sta_connect_seq, false /* copy */) !=
          NULL);
}

bool mgos_wifi_dev_sta_disconnect(void) {
  struct ism43xxx_ctx *c = (struct ism43xxx_ctx *) s_ctx;
  if (c->mode != ISM43XXX_MODE_STA) return false;
  mgos_clear_timer(s_poll_timer_id);
  s_poll_timer_id = MGOS_INVALID_TIMER_ID;
  return (ism43xxx_send_seq(c, ism43xxx_sta_disconnect_seq, false /* copy */) !=
          NULL);
}

bool mgos_wifi_dev_get_ip_info(int if_instance,
                               struct mgos_net_ip_info *ip_info) {
  struct ism43xxx_ctx *c = (struct ism43xxx_ctx *) s_ctx;
  bool res = false;
  if (if_instance == 0) {
    memcpy(ip_info, &c->sta_ip_info, sizeof(*ip_info));
    res = true;
  } else if (if_instance == 1) {
    memcpy(ip_info, &c->ap_ip_info, sizeof(*ip_info));
    res = true;
  } else {
    memset(ip_info, 0, sizeof(*ip_info));
  }
  return res;
}

char *mgos_wifi_get_sta_default_dns(void) {
  /* I don't think this is even possible... */
  return NULL;
}

int mgos_wifi_sta_get_rssi(void) {
  struct ism43xxx_ctx *c = (struct ism43xxx_ctx *) s_ctx;
  return c->sta_rssi;
}

static bool ism43xxx_mr_cb(struct ism43xxx_ctx *c,
                           const struct ism43xxx_cmd *cmd, bool ok,
                           struct mg_str p) {
  s_poll_in_progress = false;
  if (!ok) return false;
  (void) c;
  (void) cmd;
  (void) p;
  return true;
}

const struct ism43xxx_cmd ism43xxx_ap_poll_seq[] = {
    {.cmd = "AR", .ph = ism43xxx_ar_cb},
    {.cmd = "MR", .ph = ism43xxx_mr_cb},
    {.cmd = NULL},
};

const struct ism43xxx_cmd ism43xxx_sta_poll_short_seq[] = {
    {.cmd = "CS", .ph = ism43xxx_cs_cb},
    {.cmd = "MR", .ph = ism43xxx_mr_cb},
    {.cmd = NULL},
};

/* "CR" takes quite a long time to execute, about 300 ms.
 * So we only send it once in a while. */
const struct ism43xxx_cmd ism43xxx_sta_poll_long_seq[] = {
    {.cmd = "CS", .ph = ism43xxx_cs_cb},
    {.cmd = "CR", .ph = ism43xxx_cr_cb},
    {.cmd = "MR", .ph = ism43xxx_mr_cb},
    {.cmd = NULL},
};

static void ism43xxx_wifi_poll_timer_cb(void *arg) {
  if (s_ctx == NULL || s_poll_in_progress) return;
  const struct ism43xxx_cmd *seq = NULL;
  switch (s_ctx->mode) {
    case ISM43XXX_MODE_AP: {
      seq = ism43xxx_ap_poll_seq;
      break;
    }
    case ISM43XXX_MODE_STA: {
      static uint8_t s_long_ctr = 0;
      if (++s_long_ctr % 16 == 0) {
        seq = ism43xxx_sta_poll_long_seq;
      } else {
        seq = ism43xxx_sta_poll_short_seq;
      }
      break;
    }
    default:
      return;
  }
  s_poll_in_progress =
      (ism43xxx_send_seq(s_ctx, seq, false /* copy */) != NULL);
  (void) arg;
}

void mgos_wifi_dev_init(void) {
  struct ism43xxx_ctx *c = (struct ism43xxx_ctx *) calloc(1, sizeof(*c));
  const struct mgos_config_wifi_ism43xxx *cfg =
      mgos_sys_config_get_wifi_ism43xxx();
  if (cfg->spi == NULL) {
    c->spi = mgos_spi_get_global();
    if (c->spi == NULL) {
      LOG(LL_ERROR, ("SPI is not configured, make sure spi.enable is true"));
      return;
    }
  } else {
    struct mgos_config_spi spi_cfg = {
        .enable = true, .cs0_gpio = -1, .cs1_gpio = -1, .cs2_gpio = -1};
    if (!mgos_spi_config_from_json(mg_mk_str(cfg->spi), &spi_cfg) ||
        (c->spi = mgos_spi_create(&spi_cfg)) == NULL) {
      LOG(LL_ERROR, ("Invalid SPI cfg"));
      return;
    }
  }
  c->spi_freq = cfg->spi_freq;
  c->cs_gpio = cfg->cs_gpio;
  c->rst_gpio = cfg->rst_gpio;
  c->drdy_gpio = cfg->drdy_gpio;
  c->boot0_gpio = cfg->boot0_gpio;
  c->wakeup_gpio = cfg->wakeup_gpio;

  if (ism43xxx_init(c)) {
    s_ctx = c;
  }
}

bool mgos_wifi_dev_start_scan(void) {
  /* TODO(rojer) */
  LOG(LL_ERROR, ("Scanning is not implemented!"));
  return false;
}

void mgos_wifi_dev_deinit(void) {
  if (s_ctx != NULL) ism43xxx_reset(s_ctx, true /* hold */);
}

bool mgos_wifi_ism43xxx_init(void) {
  /* Real init happens in mgos_wifi_dev_init */
  return true;
}

struct ism43xxx_ctx *ism43xxx_get_ctx(void) {
  return s_ctx;
}
