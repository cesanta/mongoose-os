/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include <inttypes.h>

/* Driverlib includes */
#include "hw_types.h"

#include "hw_ints.h"
#include "hw_memmap.h"
#include "interrupt.h"
#include "pin.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"
#include "uart.h"
#include "utils.h"

/*
 * We want to disable SL_INC_STD_BSD_API_NAMING, so we include user.h ourselves
 * and undef it.
 */
#define PROVISIONING_API_H_
#include <simplelink/user.h>
#undef PROVISIONING_API_H_
#undef SL_INC_STD_BSD_API_NAMING

#undef __CONCAT
#include <simplelink/include/simplelink.h>

#include "fs.h"

#include "boot.h"

#define BOOT_CFG_0 "mg-boot.cfg.0"
#define BOOT_CFG_1 "mg-boot.cfg.1"

static int read_cfg(const char *fn, struct boot_cfg *cfg) {
  memset(cfg, 0, sizeof(*cfg));
  _i32 fh;
  _i32 r = sl_FsOpen((const _u8 *) fn, FS_MODE_OPEN_READ, NULL, &fh);
  if (r != 0) return r;
  r = sl_FsRead(fh, 0, (_u8 *) cfg, sizeof(*cfg));
  r = (r == sizeof(*cfg) ? 0 : -1000);
  sl_FsClose(fh, NULL, NULL, 0);
  return r;
}

int get_active_boot_cfg(struct boot_cfg *cfg) {
  struct boot_cfg cfg0, cfg1, *c;
  read_cfg(BOOT_CFG_0, &cfg0);
  read_cfg(BOOT_CFG_1, &cfg1);
  if (cfg0.seq > 0 && cfg1.seq > 0) {
    c = (cfg0.seq < cfg1.seq ? &cfg0 : &cfg1);
  } else if (cfg0.seq > 0) {
    c = &cfg0;
  } else if (cfg1.seq > 0) {
    c = &cfg1;
  } else {
    return -1;
  }
  if (cfg != NULL) memcpy(cfg, c, sizeof(*cfg));
  return (c == &cfg1);
}

int get_inactive_boot_cfg_idx() {
  int active = get_active_boot_cfg(NULL);
  if (active < 0) {
    return 0; /* Nothing is configured? Oh well, use 0 then. */
  }
  return (active == 0 ? 1 : 0);
}
