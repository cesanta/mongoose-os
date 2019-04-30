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

#pragma once

#include <stdbool.h>

#include "common/cs_dbg.h"

#include "mgos_bt_gatt.h"

#ifdef __cplusplus
extern "C" {
#endif

enum cs_log_level ll_from_status(esp_bt_status_t status);

bool esp32_bt_is_scanning(void);
bool esp32_bt_gattc_init(void);

bool esp32_bt_gap_init(void);
bool esp32_bt_gatts_init(void);
void esp32_bt_gatts_auth_cmpl(const esp_bd_addr_t addr, bool success);

void esp32_bt_set_is_advertising(bool is_advertising);

/* Workaround for https://github.com/espressif/esp-idf/issues/1406 */
bool esp32_bt_wipe_config(void);

esp_gatt_status_t esp32_bt_gatt_get_status(enum mgos_bt_gatt_status st);

#ifdef __cplusplus
}
#endif
