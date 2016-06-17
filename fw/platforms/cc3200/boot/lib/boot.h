/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_H_
#define CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_H_

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

/*
 * Reads currently active boot config.
 * Returns config index (zero-based) and fills *cfg,
 * returns negative value on error.
 */
int get_active_boot_cfg(struct boot_cfg *cfg);

/*
 * Returns index of an inactive boot config that can be used.
 */
int get_inactive_boot_cfg_idx();

#endif /* CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_H_ */
