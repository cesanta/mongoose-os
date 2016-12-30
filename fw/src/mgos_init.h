/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_INIT_H_
#define CS_FW_SRC_MGOS_INIT_H_

#include "fw/src/mgos_features.h"

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
  MGOS_INIT_ATCA_FAILED = -20,
  MGOS_INIT_APPLY_UPDATE_FAILED = -21,
  MGOS_INIT_CONSOLE_INIT_FAILED = -22,
  MGOS_INIT_GPIO_INIT_FAILED = -23,
};

enum mgos_init_result mgos_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MGOS_INIT_H_ */
