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

#include "esp_exc.h"

#ifdef __cplusplus
extern "C" {
#endif

void esp_dump_core(uint32_t cause, struct regfile *);

void esp_core_dump_init(void);

void esp_core_dump_set_flash_area(uint32_t addr, uint32_t max_size);

#ifdef __cplusplus
}
#endif
