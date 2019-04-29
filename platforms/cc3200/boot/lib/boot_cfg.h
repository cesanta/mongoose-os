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

#ifndef CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_CFG_H_
#define CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_CFG_H_

#include <stdint.h>

#include "cc3200_vfs_dev_slfs_container_meta.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Boot loader configuration struct, to be stored in BOOT_CFG_{0,1}.
 * Little-endian.
 */
#define MAX_APP_IMAGE_FILE_LEN 50

struct boot_cfg {
  uint64_t seq;
  uint32_t flags;
  char app_image_file[MAX_APP_IMAGE_FILE_LEN];
  uint32_t app_load_addr;
  char fs_container_prefix[MAX_FS_CONTAINER_PREFIX_LEN];
};

#define BOOT_CFG_TOMBSTONE_SEQ (~(0ULL))
#define BOOT_CFG_INITIAL_SEQ (~(0ULL) - 1ULL)

#define BOOT_F_FIRST_BOOT (1UL << 0)
#define BOOT_F_MERGE_SPIFFS (1UL << 1)
#define BOOT_F_INVALID (1UL << 31)

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_CFG_H_ */
