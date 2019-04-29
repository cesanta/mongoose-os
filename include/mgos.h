/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
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

#ifndef CS_FW_INCLUDE_MGOS_H_
#define CS_FW_INCLUDE_MGOS_H_

#include <stdbool.h>

#include "common/platform.h"

#include "common/cs_dbg.h"
#include "common/cs_file.h"
#include "common/json_utils.h"
#include "common/mbuf.h"
#include "common/str_util.h"
#ifdef __cplusplus
#ifndef __TI_COMPILER_VERSION__
#include "common/util/status.h"
#include "common/util/statusor.h"
#endif
#endif

#include "mgos_app.h"
#include "mgos_bitbang.h"
#include "mgos_config_util.h"
#include "mgos_debug.h"
#include "mgos_dlsym.h"
#include "mgos_event.h"
#include "mgos_features.h"
#include "mgos_gpio.h"
#include "mgos_init.h"
#include "mgos_mongoose.h"
#include "mgos_net.h"
#include "mgos_sys_config.h"
#include "mgos_system.h"
#include "mgos_time.h"
#include "mgos_timers.h"
#include "mgos_uart.h"
#include "mgos_utils.h"
#ifdef MGOS_HAVE_WIFI
#include "mgos_wifi.h"
#endif

/* no_extern_c_check */
#endif /* CS_FW_INCLUDE_MGOS_H_ */
