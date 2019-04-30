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

#include "rs14100_wifi.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "common/cs_dbg.h"
#include "common/cs_file.h"

#include "mgos.h"
#include "mgos_lwip.h"
#include "mgos_net_hal.h"
#include "mgos_sys_config.h"
#include "mgos_wifi.h"
#include "mgos_wifi_hal.h"

#include "rs14100_sdk.h"

static void rs14100_wifi_input(uint16_t status, uint8_t *buffer,
                               const uint32_t length) {
  if (status != RSI_SUCCESS) return;
  rs14100_wifi_sta_input(buffer, length);
}

bool mgos_wifi_dev_ap_setup(const struct mgos_config_wifi_ap *cfg) {
  if (!cfg->enable) {
    return true;
  }
  // TODO(rojer)
  return true;
}

bool mgos_wifi_dev_get_ip_info(int if_instance,
                               struct mgos_net_ip_info *ip_info) {
  switch (if_instance) {
    case MGOS_NET_IF_WIFI_STA: {
      return rs14100_wifi_sta_get_ip_info(ip_info);
    }
  }
  return false;
}

void mgos_wifi_dev_init(void) {
  NETIF_DECLARE_EXT_CALLBACK(s_rs14100_wifi_sta_ext_cb);
  netif_add_ext_callback(&s_rs14100_wifi_sta_ext_cb,
                         rs14100_wifi_sta_ext_cb_tcpip);
  rsi_wlan_register_callbacks(RSI_WLAN_DATA_RECEIVE_NOTIFY_CB,
                              rs14100_wifi_input);
  // We always start in band 0 (2.4 GHz) because that's the default value
  // before config is loaded.
  // If config sets it to something else, we need to reset the module.
  if (rs14100_wifi_get_band() != 0) {
    // Deinit performs a soft reset.
    rsi_wireless_deinit();
    // Restore RC clk (case 00005152).
    RSI_CLK_M4ssRefClkConfig(M4CLK, ULP_32MHZ_RC_CLK);
    // Init again, this time for real.
    rsi_wireless_init(RSI_WLAN_CLIENT_MODE, 0);
    rsi_wlan_radio_init();
  }
  const struct mgos_config_wifi_sta_params_bg_scan *bcfg =
      mgos_sys_config_get_wifi_sta_params_bg_scan();
  g_wifi_sta_bg_scan_params.enable = &bcfg->enable;
  g_wifi_sta_bg_scan_params.period = &bcfg->period;
  g_wifi_sta_bg_scan_params.rssi_threshold = &bcfg->rssi_threshold;
  g_wifi_sta_bg_scan_params.rssi_tolerance = &bcfg->rssi_tolerance;
  g_wifi_sta_bg_scan_params.active_duration_ms = &bcfg->active_duration_ms;
  g_wifi_sta_bg_scan_params.passive_duration_ms = &bcfg->passive_duration_ms;
  const struct mgos_config_wifi_sta_params_roaming *rcfg =
      mgos_sys_config_get_wifi_sta_params_roaming();
  g_wifi_sta_roaming_params.enable = &rcfg->enable;
  g_wifi_sta_roaming_params.rssi_threshold = &rcfg->rssi_threshold;
  g_wifi_sta_roaming_params.rssi_hysteresis = &rcfg->rssi_hysteresis;
}

void mgos_wifi_dev_deinit(void) {
  rsi_wireless_deinit();
}
