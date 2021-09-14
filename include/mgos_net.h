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

/*
 * Low-level network configuration API.
 *
 * Contains definitions of the configuration state. Allows to set up an
 * event handler that tracks state changes: when the network connectivity
 * is lost, established, etc.
 */

#pragma once

#include <stdbool.h>

#include "mongoose.h"

#include "mgos_config.h"
#include "mgos_event.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Event group which should be given to `mgos_event_add_group_handler()`
 * in order to subscribe to network events.
 *
 * Example:
 * ```c
 * static void my_net_ev_handler(int ev, void *evd, void *arg) {
 *   if (ev == MGOS_NET_EV_IP_ACQUIRED) {
 *     LOG(LL_INFO, ("Just got IP!"));
 *     // Fetch something very useful from somewhere
 *   }
 *   (void) evd;
 *   (void) arg;
 * }
 *
 * // Somewhere else:
 * mgos_event_add_group_handler(MGOS_EVENT_GRP_NET, my_net_ev_handler, NULL);
 * ```
 */
#define MGOS_EVENT_GRP_NET MGOS_EVENT_BASE('N', 'E', 'T')

/*
 * Event types which are delivered to the callback registered with
 * `mgos_event_add_handler()` or `mgos_event_add_group_handler()`, see
 * example in the documentation for `MGOS_EVENT_GRP_NET`.
 */
enum mgos_net_event {
  MGOS_NET_EV_DISCONNECTED = MGOS_EVENT_GRP_NET,
  MGOS_NET_EV_CONNECTING,
  MGOS_NET_EV_CONNECTED,
  MGOS_NET_EV_IP_ACQUIRED,
};

/*
 * Interface type
 */
enum mgos_net_if_type {
  MGOS_NET_IF_TYPE_WIFI,
  MGOS_NET_IF_TYPE_ETHERNET,
  MGOS_NET_IF_TYPE_PPP,
  /* This is a sentinel in case all networking interface types are disabled. */
  MGOS_NET_IF_MAX,
};

struct mgos_net_ip_info {
  struct sockaddr_in ip;
  struct sockaddr_in netmask;
  struct sockaddr_in gw;
  struct sockaddr_in dns;
  struct sockaddr_in ntp;
};

struct mgos_net_event_data {
  enum mgos_net_if_type if_type;
  int if_instance;
  struct mgos_net_ip_info ip_info;
};

/*
 * Retrieve IP configuration of the provided interface type and instance
 * number, and fill provided `ip_info` with it. Returns `true` in case of
 * success, false otherwise.
 */
bool mgos_net_get_ip_info(enum mgos_net_if_type if_type, int if_instance,
                          struct mgos_net_ip_info *ip_info);

/*
 * Converts address to dotted-quad NUL-terminated string.
 * `out` must be at least 16 bytes long.
 * Returns the out pointer.
 */
char *mgos_net_ip_to_str(const struct sockaddr_in *sin, char *out);

/*
 * Parses dotted-quad NUL-terminated string into an IPv4 address.
 */
bool mgos_net_str_to_ip(const char *ips, struct sockaddr_in *sin);

/*
 * Parses dotted-quad NUL-terminated string into an IPv4 address.
 */
bool mgos_net_str_to_ip_n(const struct mg_str ips, struct sockaddr_in *sin);

#ifdef __cplusplus
}
#endif
