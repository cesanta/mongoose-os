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
 * HAL wifi interface, to be implemented by ports.
 */

#ifndef CS_MOS_LIBS_WIFI_SRC_MGOS_WIFI_HAL_H_
#define CS_MOS_LIBS_WIFI_SRC_MGOS_WIFI_HAL_H_

#include "mgos_net.h"
#include "mgos_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

bool mgos_wifi_dev_ap_setup(const struct mgos_config_wifi_ap *cfg);

bool mgos_wifi_dev_sta_setup(const struct mgos_config_wifi_sta *cfg);
bool mgos_wifi_dev_sta_connect(void); /* To the previously _setup network. */
bool mgos_wifi_dev_sta_disconnect(void);
enum mgos_wifi_status mgos_wifi_dev_sta_get_status(void);

bool mgos_wifi_dev_get_ip_info(int if_instance,
                               struct mgos_net_ip_info *ip_info);

struct mgos_wifi_dev_event_info {
  enum mgos_wifi_event ev;
  union {
    struct mgos_wifi_sta_connected_arg sta_connected;
    struct mgos_wifi_sta_disconnected_arg sta_disconnected;
    struct mgos_wifi_ap_sta_connected_arg ap_sta_connected;
    struct mgos_wifi_ap_sta_disconnected_arg ap_sta_disconnected;
  };
};

/* Invoke this when Wifi connection state changes. */
void mgos_wifi_dev_event_cb(const struct mgos_wifi_dev_event_info *dei);

bool mgos_wifi_dev_start_scan(void);
/*
 * Invoke this when the scan is done. In case of error, pass num_res < 0.
 * If res is non-NULL, it must be heap-allocated and mgos_wifi takes it over.
 * It is explicitly allowed to invoke mgos_wifi_dev_scan_cb from within
 * mgos_wifi_dev_start_scan if results are available immediately.
 */
void mgos_wifi_dev_scan_cb(int num_res, struct mgos_wifi_scan_result *res);

void mgos_wifi_dev_init(void);
void mgos_wifi_dev_deinit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_MOS_LIBS_WIFI_SRC_MGOS_WIFI_HAL_H_ */
