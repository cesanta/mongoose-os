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

#ifndef CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_H_
#define CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_H_

#include "boot_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Returns index of the currently active boot config.
 * Returns negative value if there isn't one or an error occurs.
 */
int get_active_boot_cfg_idx(void);

/*
 * Returns index of an inactive boot config that can be used.
 */
int get_inactive_boot_cfg_idx(void);

/*
 * Reads boot config with the given index.
 * Returns config index (zero-based) and fills *cfg,
 * returns negative value on error.
 */
int read_boot_cfg(int idx, struct boot_cfg *cfg);

/*
 * Writes boot config to the slot with the given index.
 * Returns negative value on error.
 */
int write_boot_cfg(const struct boot_cfg *cfg, int idx);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_H_ */
