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

#ifndef CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_META_H_
#define CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_META_H_

#include "boot_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

union boot_cfg_meta {
  struct boot_cfg cfg;
  uint8_t padding[512];
};

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_META_H_ */
