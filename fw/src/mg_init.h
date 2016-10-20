/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MG_INIT_H_
#define CS_FW_SRC_MG_INIT_H_

#include "fw/src/mg_features.h"

enum mg_init_result {
  MG_INIT_OK = 0,
  MG_INIT_OUT_OF_MEMORY = -1,
  MG_INIT_APP_INIT_FAILED = -2,
  MG_INIT_APP_JS_INIT_FAILED = -3,
  MG_INIT_SYS_INIT_JS_FAILED = -4,
  MG_INIT_CONFIG_LOAD_DEFAULTS_FAILED = -10,
  MG_INIT_CONFIG_WIFI_INIT_FAILED = -11,
  MG_INIT_CONFIG_INVALID_STDOUT_UART = -12,
  MG_INIT_CONFIG_INVALID_STDERR_UART = -13,
  MG_INIT_CONFIG_WEB_SERVER_LISTEN_FAILED = -14,
  MG_INIT_CLUBBY_FAILED = -15,
  MG_INIT_UART_FAILED = -16,
  MG_INIT_MDNS_FAILED = -17,
};

enum mg_init_result mg_init(void);

#endif /* CS_FW_SRC_MG_INIT_H_ */
