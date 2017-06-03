/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_H_
#define CS_FW_SRC_MGOS_H_

#include <stdbool.h>

#include "common/platform.h"

#include "common/cs_dbg.h"
#include "common/cs_file.h"

#include "fw/src/mgos_adc.h"
#include "fw/src/mgos_app.h"
#include "fw/src/mgos_arduino.h"
#include "fw/src/mgos_atca.h"
#include "fw/src/mgos_bitbang.h"
#include "fw/src/mgos_config.h"
#include "fw/src/mgos_console.h"
#include "fw/src/mgos_debug.h"
#include "fw/src/mgos_debug_hal.h"
#include "fw/src/mgos_deps.h"
#include "fw/src/mgos_dlsym.h"
#include "fw/src/mgos_dns_sd.h"
#include "fw/src/mgos_features.h"
#include "fw/src/mgos_gpio.h"
#include "fw/src/mgos_gpio_hal.h"
#include "fw/src/mgos_gpio_service.h"
#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_hooks.h"
#include "fw/src/mgos_i2c.h"
#include "fw/src/mgos_i2c_service.h"
#include "fw/src/mgos_init.h"
#include "fw/src/mgos_mdns.h"
#include "fw/src/mgos_mongoose.h"
#include "fw/src/mgos_onewire.h"
#include "fw/src/mgos_pwm.h"
#include "fw/src/mgos_rpc.h"
#include "fw/src/mgos_rpc_channel_uart.h"
#include "fw/src/mgos_service_config.h"
#include "fw/src/mgos_service_filesystem.h"
#include "fw/src/mgos_sntp.h"
#include "fw/src/mgos_spi.h"
#include "fw/src/mgos_sys_config.h"
#include "fw/src/mgos_timers.h"
#include "fw/src/mgos_uart.h"
#include "fw/src/mgos_uart_hal.h"
#include "fw/src/mgos_updater.h"
#include "fw/src/mgos_updater_common.h"
#include "fw/src/mgos_updater_hal.h"
#include "fw/src/mgos_updater_http.h"
#include "fw/src/mgos_updater_rpc.h"
#include "fw/src/mgos_updater_util.h"
#include "fw/src/mgos_utils.h"
#include "fw/src/mgos_wifi.h"

#endif
