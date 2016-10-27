/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_CFG_H_
#define CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_CFG_H_

#include <inttypes.h>

/*
 * Boot loader configuration struct, to be stored in BOOT_CFG_{0,1}.
 * Little-endian.
 */
#define MAX_APP_IMAGE_FILE_LEN 50
#define MAX_FS_CONTAINER_PREFIX_LEN 50

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

#endif /* CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_CFG_H_ */
