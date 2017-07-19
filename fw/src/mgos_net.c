/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "mgos_net.h"
#include "mgos_net_hal.h"

#include "common/cs_dbg.h"
#include "common/queue.h"

#include "mgos_hal.h"
#include "mgos_mongoose.h"
#include "mgos_wifi_hal.h"

struct cb_info {
  mgos_net_event_handler_t cb;
  void *arg;
  SLIST_ENTRY(cb_info) next;
};
static SLIST_HEAD(s_cbs, cb_info) s_cbs;

struct net_ev_info {
  enum mgos_net_if_type if_type;
  int if_instance;
  enum mgos_net_event ev;
};

struct mgos_rlock_type *s_net_lock = NULL;

static inline void net_lock(void) {
  mgos_rlock(s_net_lock);
}

static inline void net_unlock(void) {
  mgos_runlock(s_net_lock);
}

static const char *get_if_name(enum mgos_net_if_type if_type, int if_instance) {
  const char *name = "";
  switch (if_type) {
#if MGOS_ENABLE_WIFI
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
#endif
#ifdef MGOS_HAVE_ETHERNET
    case MGOS_NET_IF_TYPE_ETHERNET: {
      name = "ETH";
      break;
    }
#endif
  }
  (void) if_instance;
  return name;
}

static void mgos_net_on_change_cb(void *arg) {
  struct net_ev_info *ei = (struct net_ev_info *) arg;
  const char *if_name = get_if_name(ei->if_type, ei->if_instance);
  struct mgos_net_event_data evd = {
      .if_type = ei->if_type, .if_instance = ei->if_instance,
  };
  switch (ei->ev) {
    case MGOS_NET_EV_DISCONNECTED: {
      LOG(LL_INFO, ("%s: disconnected", if_name));
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
        char ip[16], gw[16];
        char *nameserver = mgos_get_nameserver();
        memset(ip, 0, sizeof(ip));
        memset(gw, 0, sizeof(gw));
        mgos_net_ip_to_str(&evd.ip_info.ip, ip);
        mgos_net_ip_to_str(&evd.ip_info.gw, gw);
        LOG(LL_INFO, ("%s: ready, IP %s, GW %s, DNS %s", if_name, ip, gw,
                      nameserver ? nameserver : "default"));
        mg_set_nameserver(mgos_get_mgr(), nameserver);
        free(nameserver);
      }
      break;
    }
  }
  net_lock();
  struct cb_info *e, *te;
  SLIST_FOREACH_SAFE(e, &s_cbs, next, te) {
    net_unlock();
    e->cb(ei->ev, &evd, e->arg);
    net_lock();
  }
  net_unlock();
  free(ei);
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

void mgos_net_add_event_handler(mgos_net_event_handler_t eh, void *arg) {
  struct cb_info *e = (struct cb_info *) calloc(1, sizeof(*e));
  if (e == NULL) return;
  e->cb = eh;
  e->arg = arg;
  net_lock();
  SLIST_INSERT_HEAD(&s_cbs, e, next);
  net_unlock();
}

bool mgos_net_get_ip_info(enum mgos_net_if_type if_type, int if_instance,
                          struct mgos_net_ip_info *ip_info) {
  switch (if_type) {
#if MGOS_ENABLE_WIFI
    case MGOS_NET_IF_TYPE_WIFI: {
      return mgos_wifi_dev_get_ip_info(if_instance, ip_info);
    }
#endif
#ifdef MGOS_HAVE_ETHERNET
    case MGOS_NET_IF_TYPE_ETHERNET: {
      return mgos_eth_dev_get_ip_info(if_instance, ip_info);
    }
#endif
  }
  return false;
}

void mgos_net_ip_to_str(const struct sockaddr_in *sin, char *out) {
  union socket_address sa;
  sa.sa.sa_family = AF_INET;
  memcpy(&sa.sin, sin, sizeof(sa.sin));
  mg_sock_addr_to_str(&sa, out, 16, MG_SOCK_STRINGIFY_IP);
}

enum mgos_init_result mgos_net_init(void) {
  s_net_lock = mgos_new_rlock();
  return MGOS_INIT_OK;
}
