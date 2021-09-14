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

#include "mgos_net_hal.h"
#include "mgos_net_internal.h"

#include "common/cs_dbg.h"
#include "common/queue.h"

#include "mgos_event.h"
#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_system.h"
#ifdef MGOS_HAVE_PPPOS
#include "mgos_pppos.h"
#endif
#ifdef MGOS_HAVE_WIFI
#include "mgos_wifi_hal.h"
#endif

struct net_ev_info {
  enum mgos_net_if_type if_type;
  int if_instance;
  enum mgos_net_event ev;
};

static void mgos_update_nameserver(void);

static const char *get_if_name(enum mgos_net_if_type if_type, int if_instance) {
  const char *name = "";
  switch (if_type) {
    case MGOS_NET_IF_TYPE_WIFI: {
      switch (if_instance) {
        case 0: {
          name = "WiFi STA";
          break;
        }
        case 1: {
          name = "WiFi AP";
          break;
        }
      }
      break;
    }
    case MGOS_NET_IF_TYPE_ETHERNET: {
      name = "ETH";
      break;
    }
    case MGOS_NET_IF_TYPE_PPP: {
      name = "PPP";
      break;
    }
    case MGOS_NET_IF_MAX: {
      break;
    }
  }
  (void) if_instance;
  return name;
}

static void mgos_net_on_change_cb(void *arg) {
  struct net_ev_info *ei = (struct net_ev_info *) arg;
  const char *if_name = get_if_name(ei->if_type, ei->if_instance);
  struct mgos_net_event_data evd = {
      .if_type = ei->if_type,
      .if_instance = ei->if_instance,
  };
  switch (ei->ev) {
    case MGOS_NET_EV_DISCONNECTED: {
      LOG(LL_INFO, ("%s: disconnected", if_name));
      mgos_update_nameserver();
      break;
    }
    case MGOS_NET_EV_CONNECTING: {
      LOG(LL_INFO, ("%s: connecting", if_name));
      break;
    }
    case MGOS_NET_EV_CONNECTED: {
      LOG(LL_INFO, ("%s: connected", if_name));
      break;
    }
    case MGOS_NET_EV_IP_ACQUIRED: {
      if (mgos_net_get_ip_info(ei->if_type, ei->if_instance, &evd.ip_info)) {
        char ip[16], gw[16], dns[16], ntp[16];
        mgos_net_ip_to_str(&evd.ip_info.ip, ip);
        mgos_net_ip_to_str(&evd.ip_info.gw, gw);
        mgos_net_ip_to_str(&evd.ip_info.dns, dns);
        mgos_net_ip_to_str(&evd.ip_info.ntp, ntp);
        LOG(LL_INFO, ("%s: ready, IP %s, GW %s, DNS %s, NTP %s", if_name, ip,
                      gw, dns, ntp));
      }
      mgos_update_nameserver();
      break;
    }
  }

  mgos_event_trigger(ei->ev, &evd);

  free(ei);
  (void) if_name;
}

void mgos_net_dev_event_cb(enum mgos_net_if_type if_type, int if_instance,
                           enum mgos_net_event ev) {
  struct net_ev_info *ei = (struct net_ev_info *) calloc(1, sizeof(*ei));
  if (ei == NULL) return;
  ei->if_type = if_type;
  ei->if_instance = if_instance;
  ei->ev = ev;
  mgos_invoke_cb(mgos_net_on_change_cb, ei, false /* from_isr */);
}

bool mgos_net_get_ip_info(enum mgos_net_if_type if_type, int if_instance,
                          struct mgos_net_ip_info *ip_info) {
  switch (if_type) {
    case MGOS_NET_IF_TYPE_WIFI:
#ifdef MGOS_HAVE_WIFI
      return mgos_wifi_dev_get_ip_info(if_instance, ip_info);
#else
      return false;
#endif
    case MGOS_NET_IF_TYPE_ETHERNET:
#ifdef MGOS_HAVE_ETHERNET
      return mgos_eth_dev_get_ip_info(if_instance, ip_info);
#else
      return false;
#endif
    case MGOS_NET_IF_TYPE_PPP:
#ifdef MGOS_HAVE_PPPOS
      return mgos_pppos_dev_get_ip_info(if_instance, ip_info);
#else
      return false;
#endif
    case MGOS_NET_IF_MAX: {
      (void) if_type;
      (void) if_instance;
      (void) ip_info;
      break;
    }
  }
  return false;
}

char *mgos_net_ip_to_str(const struct sockaddr_in *sin, char *out) {
  union socket_address sa;
  sa.sa.sa_family = AF_INET;
  memcpy(&sa.sin, sin, sizeof(sa.sin));
  mg_sock_addr_to_str(&sa, out, 16, MG_SOCK_STRINGIFY_IP);
  return out;
}

bool mgos_net_str_to_ip(const char *ips, struct sockaddr_in *sin) {
  unsigned int a, b, c, d;
  if (sscanf(ips, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
    sin->sin_addr.s_addr = 0;
    return false;
  }
  sin->sin_addr.s_addr = htonl((a << 24) | (b << 16) | (c << 8) | d);
  return true;
}

bool mgos_net_str_to_ip_n(struct mg_str ips, struct sockaddr_in *sin) {
  ips = mg_strdup_nul(ips);
  bool res = mgos_net_str_to_ip(ips.p, sin);
  free((void *) ips.p);
  return res;
}

static void mgos_update_nameserver(void) {
  char nameserver[16];
  struct mgos_net_ip_info ip_info;
  memset(&ip_info, 0, sizeof(ip_info));
  if (mgos_net_get_ip_info(MGOS_NET_IF_TYPE_ETHERNET, 0, &ip_info) &&
      ip_info.ip.sin_addr.s_addr != 0 && ip_info.dns.sin_addr.s_addr != 0) {
    goto out;
  }
  memset(&ip_info, 0, sizeof(ip_info));
  if (mgos_net_get_ip_info(MGOS_NET_IF_TYPE_WIFI, 0, &ip_info) &&
      ip_info.ip.sin_addr.s_addr != 0 && ip_info.dns.sin_addr.s_addr != 0) {
    goto out;
  }
  memset(&ip_info, 0, sizeof(ip_info));
  if (mgos_net_get_ip_info(MGOS_NET_IF_TYPE_PPP, 0, &ip_info) &&
      ip_info.ip.sin_addr.s_addr != 0 && ip_info.dns.sin_addr.s_addr != 0) {
    goto out;
  }
  mgos_net_str_to_ip(MGOS_DEFAULT_NAMESERVER, &ip_info.dns);
out:
  mgos_net_ip_to_str(&ip_info.dns, nameserver);
  struct mg_mgr *mgr = mgos_get_mgr();
  if (mgr->nameserver != NULL && strcmp(mgr->nameserver, nameserver) == 0)
    return;
  LOG(LL_DEBUG, ("Setting %s server to %s", "DNS", nameserver));
  mg_set_nameserver(mgr, nameserver);
}

enum mgos_init_result mgos_net_init(void) {
  if (!mgos_event_register_base(MGOS_EVENT_GRP_NET, "net")) {
    return MGOS_INIT_NET_INIT_FAILED;
  }

  return MGOS_INIT_OK;
}
