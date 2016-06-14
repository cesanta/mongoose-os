/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_BOOT_SRC_BOOT_H_
#define CS_FW_PLATFORMS_CC3200_BOOT_SRC_BOOT_H_

#include <inttypes.h>

#define BOOT_CFG_INITIAL_SEQ (~(0ULL) - 1ULL)

/*
 * Boot loader configuration struct, to be stored in BOOT_CFG_{0,1}.
 * Little-endian.
 */

struct boot_cfg {
  char image_file[50];
  uint32_t base_address;
  uint64_t seq;
};

#define BOOT_CFG_0 "mg-boot.cfg.0"
#define BOOT_CFG_1 "mg-boot.cfg.1"

#endif /* CS_FW_PLATFORMS_CC3200_BOOT_SRC_BOOT_H_ */
