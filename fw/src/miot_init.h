/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MIOT_INIT_H_
#define CS_FW_SRC_MIOT_INIT_H_

#include "fw/src/miot_features.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

enum miot_init_result {
  MIOT_INIT_OK = 0,
  MIOT_INIT_OUT_OF_MEMORY = -1,
  MIOT_INIT_APP_INIT_FAILED = -2,
  MIOT_INIT_APP_JS_INIT_FAILED = -3,
  MIOT_INIT_SYS_INIT_JS_FAILED = -4,
  MIOT_INIT_FS_INIT_FAILED = -5,
  MIOT_INIT_CONFIG_LOAD_DEFAULTS_FAILED = -10,
  MIOT_INIT_CONFIG_WIFI_INIT_FAILED = -11,
  MIOT_INIT_CONFIG_INVALID_STDOUT_UART = -12,
  MIOT_INIT_CONFIG_INVALID_STDERR_UART = -13,
  MIOT_INIT_CONFIG_WEB_SERVER_LISTEN_FAILED = -14,
  MIOT_INIT_MG_RPC_FAILED = -15,
  MIOT_INIT_UART_FAILED = -16,
  MIOT_INIT_MDNS_FAILED = -17,
  MIOT_INIT_MQTT_FAILED = -18,
  MIOT_INIT_I2C_FAILED = -19,
  MIOT_INIT_ATCA_FAILED = -20,
  MIOT_INIT_APPLY_UPDATE_FAILED = -21,
  MIOT_INIT_CONSOLE_INIT_FAILED = -22,
  MIOT_INIT_GPIO_INIT_FAILED = -23,
};

enum miot_init_result miot_init(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* CS_FW_SRC_MIOT_INIT_H_ */
