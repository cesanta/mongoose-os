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

#ifndef CS_FW_INCLUDE_MGOS_INIT_H_
#define CS_FW_INCLUDE_MGOS_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum mgos_init_result {
  MGOS_INIT_OK = 0,
  MGOS_INIT_OUT_OF_MEMORY = -1,
  MGOS_INIT_APP_INIT_FAILED = -2,
  MGOS_INIT_APP_JS_INIT_FAILED = -3,
  MGOS_INIT_SYS_INIT_JS_FAILED = -4,
  MGOS_INIT_FS_INIT_FAILED = -5,
  MGOS_INIT_CONFIG_LOAD_DEFAULTS_FAILED = -10,
  MGOS_INIT_CONFIG_WIFI_INIT_FAILED = -11,
  MGOS_INIT_CONFIG_INVALID_STDOUT_UART = -12,
  MGOS_INIT_CONFIG_INVALID_STDERR_UART = -13,
  MGOS_INIT_CONFIG_WEB_SERVER_LISTEN_FAILED = -14,
  MGOS_INIT_MG_RPC_FAILED = -15,
  MGOS_INIT_UART_FAILED = -16,
  MGOS_INIT_MDNS_FAILED = -17,
  MGOS_INIT_MQTT_FAILED = -18,
  MGOS_INIT_I2C_FAILED = -19,
  MGOS_INIT_APPLY_UPDATE_FAILED = -21,
  MGOS_INIT_CONSOLE_INIT_FAILED = -22,
  MGOS_INIT_GPIO_INIT_FAILED = -23,
  MGOS_INIT_DEBUG_INIT_FAILED = -24,
  MGOS_INIT_MQTT_INIT_FAILED = -25,
  MGOS_INIT_AWS_SHADOW_INIT_FAILED = -26,
  MGOS_INIT_SNTP_INIT_FAILED = -27,
  MGOS_INIT_TIMERS_INIT_FAILED = -28,
  MGOS_INIT_SPI_FAILED = -29,
  MGOS_INIT_GCP_INIT_FAILED = -30,
  MGOS_INIT_GCP_INIT_FAILED_INVALID_KEY = -31,
  MGOS_INIT_DEPS_FAILED = -32,
  MGOS_INIT_MOUNT_FAILED = -33,
  MGOS_INIT_NET_INIT_FAILED = -34,
  MGOS_INIT_UPD_INIT_FAILED = -35,
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_INCLUDE_MGOS_INIT_H_ */
