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
// no_extern_c_check

#include "RS1xxxx.h"
#include "rsi_chip.h"
#include "rsi_rom_table_RS1xxxx.h"
#include "rsi_egpio.h"
#include "rsi_power_save.h"
#include "rsi_retention.h"
#include "rsi_rom_clks.h"
#include "rsi_rom_egpio.h"
#include "rsi_rom_ulpss_clk.h"
#include "clock_update.h"

#include "rsi_data_types.h"
#include "rsi_common_apis.h"
#include "rsi_driver.h"
#include "rsi_os.h"
#include "rsi_wlan_apis.h"

void rs14100_set_int_handler(int irqn, void (*handler)(void));
uint32_t rs14100_get_lf_fsm_clk(void);
