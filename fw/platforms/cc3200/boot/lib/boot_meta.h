#ifndef CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_META_H_
#define CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_META_H_

#include "boot_cfg.h"

union boot_cfg_meta {
  struct boot_cfg cfg;
  uint8_t padding[512];
};

#endif /* CS_FW_PLATFORMS_CC3200_BOOT_LIB_BOOT_META_H_ */
