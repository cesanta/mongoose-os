/*
 * Copyright (c) 2014-2019 Cesanta Software Limited
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

#pragma once

#include <stdbool.h>

#include "mgos_net_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

void rs14100_wifi_sta_input(const uint8_t *buffer, uint32_t length);
bool rs14100_wifi_sta_get_ip_info(struct mgos_net_ip_info *ip_info);
void rs14100_wifi_sta_ext_cb_tcpip(struct netif *netif,
                                   netif_nsc_reason_t reason,
                                   const netif_ext_callback_args_t *args);

#ifdef __cplusplus
}
#endif
