/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_H_
#define CS_FW_SRC_MGOS_H_

#include <stdbool.h>

#include "common/json_utils.h"
#include "common/platform.h"

#include "common/cs_dbg.h"
#include "common/cs_file.h"

#include "mgos_adc.h"
#include "mgos_app.h"
#include "mgos_bitbang.h"
#include "mgos_config.h"
#include "mgos_debug.h"
#include "mgos_debug_hal.h"
#include "mgos_deps.h"
#include "mgos_dlsym.h"
#include "mgos_features.h"
#include "mgos_gpio.h"
#include "mgos_gpio_hal.h"
#include "mgos_hal.h"
#include "mgos_hooks.h"
#include "mgos_init.h"
#include "mgos_mdns.h"
#include "mgos_mongoose.h"
#include "mgos_net.h"
#include "mgos_onewire.h"
#include "mgos_pwm.h"
#include "mgos_sntp.h"
#include "mgos_sys_config.h"
#include "mgos_timers.h"
#include "mgos_uart.h"
#include "mgos_uart_hal.h"
#include "mgos_updater.h"
#include "mgos_updater_common.h"
#include "mgos_updater_hal.h"
#include "mgos_updater_util.h"
#include "mgos_utils.h"
#ifdef MGOS_HAVE_WIFI
#include "mgos_wifi.h"
#endif

#endif
