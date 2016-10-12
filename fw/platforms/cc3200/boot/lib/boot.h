/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_H_
#define CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_H_

#include "boot_cfg.h"

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

#endif /* CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_H_ */
