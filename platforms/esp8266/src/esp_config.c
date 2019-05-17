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

#include <stdio.h>

#ifdef RTOS_SDK
#include <esp_common.h>
#else
#include <user_interface.h>
#endif

void device_get_mac_address(uint8_t mac[6]) {
  wifi_get_macaddr(SOFTAP_IF, mac);
}

void device_set_mac_address(uint8_t mac[6]) {
  wifi_set_macaddr(SOFTAP_IF, mac);
  mac[5] -= 2;
  wifi_set_macaddr(STATION_IF, mac);
}