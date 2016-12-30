/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_wifi.h"

#include <stdbool.h>
#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/queue.h"
#include "fw/src/mgos_sys_config.h"

struct wifi_cb {
  SLIST_ENTRY(wifi_cb) entries;
  mgos_wifi_changed_t cb;
  void *arg;
};

SLIST_HEAD(wifi_cbs, wifi_cb) s_wifi_cbs;

void mgos_wifi_on_change_cb(enum mgos_wifi_status event) {
  switch (event) {
    case MGOS_WIFI_DISCONNECTED:
      LOG(LL_INFO, ("Wifi: disconnected"));
      break;
    case MGOS_WIFI_CONNECTED:
      LOG(LL_INFO, ("Wifi: connected"));
      break;
    case MGOS_WIFI_IP_ACQUIRED: {
      char *ip = mgos_wifi_get_sta_ip();
      if (ip != NULL) {
        LOG(LL_INFO, ("WiFi: ready, IP %s", ip));
        free(ip);
      }
      break;
    }
  }

  struct wifi_cb *e, *te;
  SLIST_FOREACH_SAFE(e, &s_wifi_cbs, entries, te) {
    e->cb(event, e->arg);
  }
}

void mgos_wifi_add_on_change_cb(mgos_wifi_changed_t cb, void *arg) {
  struct wifi_cb *e = calloc(1, sizeof(*e));
  if (e == NULL) return;
  e->cb = cb;
  e->arg = arg;
  SLIST_INSERT_HEAD(&s_wifi_cbs, e, entries);
}

void mgos_wifi_remove_on_change_cb(mgos_wifi_changed_t cb, void *arg) {
  struct wifi_cb *e;
  SLIST_FOREACH(e, &s_wifi_cbs, entries) {
    if (e->cb == cb && e->arg == arg) {
      SLIST_REMOVE(&s_wifi_cbs, e, wifi_cb, entries);
      return;
    }
  }
}

bool mgos_wifi_validate_sta_cfg(const struct sys_config_wifi_sta *cfg,
                                char **msg) {
  if (!cfg->enable) return true;
  if (cfg->ssid == NULL || strlen(cfg->ssid) > 31) {
    if (!mg_asprintf(msg, 0, "%s %s must be between %d and %d chars", "STA",
                     "SSID", 1, 31)) {
    }
    return false;
  }
  if (cfg->pass != NULL && (strlen(cfg->pass) < 8 || strlen(cfg->pass) > 63)) {
    if (!mg_asprintf(msg, 0, "%s %s must be between %d and %d chars", "STA",
                     "password", 8, 63)) {
    }
    return false;
  }
  if (cfg->ip != NULL) {
    if (cfg->netmask == NULL) {
      if (!mg_asprintf(msg, 0,
                       "Station static IP is set but no netmask provided")) {
      }
      return false;
    }
    /* TODO(rojer): More validation here: IP & gw within the same net. */
  }
  return true;
}

bool mgos_wifi_validate_ap_cfg(const struct sys_config_wifi_ap *cfg,
                               char **msg) {
  if (!cfg->enable) return true;
  if (cfg->ssid == NULL || strlen(cfg->ssid) > 31) {
    if (!mg_asprintf(msg, 0, "%s %s must be between %d and %d chars", "AP",
                     "SSID", 1, 31)) {
    }
    return false;
  }
  if (cfg->pass != NULL && (strlen(cfg->pass) < 8 || strlen(cfg->pass) > 63)) {
    if (!mg_asprintf(msg, 0, "%s %s must be between %d and %d chars", "AP",
                     "password", 8, 63)) {
    }
    return false;
  }
  if (cfg->ip == NULL || cfg->netmask == NULL || cfg->dhcp_start == NULL ||
      cfg->dhcp_end == NULL) {
    *msg = strdup("AP IP, netmask, DHCP start and end addresses must be set");
    return false;
  }
  /* TODO(rojer): More validation here. DHCP range, netmask, GW (if set). */
  return true;
}

static bool validate_wifi_cfg(const struct sys_config *cfg, char **msg) {
  return (mgos_wifi_validate_ap_cfg(&cfg->wifi.ap, msg) &&
          mgos_wifi_validate_sta_cfg(&cfg->wifi.sta, msg));
}

enum mgos_init_result mgos_wifi_init(void) {
  mgos_register_config_validator(validate_wifi_cfg);
  mgos_wifi_hal_init();
  return MGOS_INIT_OK;
}
