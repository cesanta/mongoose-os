/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_NET_H_
#define CS_FW_SRC_MGOS_NET_H_

#include <stdbool.h>

#include "mongoose/mongoose.h"

#include "mgos_init.h"
#include "sys_config.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_net_event {
  MGOS_NET_EV_DISCONNECTED = 0,
  MGOS_NET_EV_CONNECTING = 1,
  MGOS_NET_EV_CONNECTED = 2,
  MGOS_NET_EV_IP_ACQUIRED = 3,
};

enum mgos_net_if_type {
#if MGOS_ENABLE_WIFI
  MGOS_NET_IF_TYPE_WIFI,
#endif
#ifdef MGOS_HAVE_ETHERNET
  MGOS_NET_IF_TYPE_ETHERNET,
#endif
};

struct mgos_net_ip_info {
  struct sockaddr_in ip;
  struct sockaddr_in netmask;
  struct sockaddr_in gw;
};

struct mgos_net_event_data {
  enum mgos_net_if_type if_type;
  int if_instance;
  struct mgos_net_ip_info ip_info;
};

typedef void (*mgos_net_event_handler_t)(
    enum mgos_net_event ev, const struct mgos_net_event_data *ev_data,
    void *arg);

void mgos_net_add_event_handler(mgos_net_event_handler_t eh, void *arg);

void mgos_net_remove_event_handler(mgos_net_event_handler_t eh, void *arg);

bool mgos_net_get_ip_info(enum mgos_net_if_type if_type, int if_instance,
                          struct mgos_net_ip_info *ip_info);

/*
 * Converts address to dotted-quad NUL-terminated string.
 * out must be at least 16 bytes long.
 */
void mgos_net_ip_to_str(const struct sockaddr_in *sin, char *out);

enum mgos_init_result mgos_net_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_NET_H_ */
