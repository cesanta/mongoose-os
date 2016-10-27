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
#include "boot_meta.h"

#define BOOT_CFG_FNAME_PREFIX "mg-boot.cfg"

int get_active_boot_cfg_idx(void) {
  struct boot_cfg cfg0, cfg1, *c;
  read_boot_cfg(0, &cfg0);
  read_boot_cfg(1, &cfg1);
  if (cfg0.seq > 0 && cfg1.seq > 0) {
    c = (cfg0.seq < cfg1.seq ? &cfg0 : &cfg1);
  } else if (cfg0.seq > 0) {
    c = &cfg0;
  } else if (cfg1.seq > 0) {
    c = &cfg1;
  } else {
    return -1;
  }
  return (c == &cfg0 ? 0 : 1);
}

int get_inactive_boot_cfg_idx(void) {
  int active = get_active_boot_cfg_idx();
  if (active < 0) {
    return 0; /* Nothing is configured? Oh well, use 0 then. */
  }
  return (active == 0 ? 1 : 0);
}

static int read_cfg(const _u8 *fn, struct boot_cfg *cfg) {
  union boot_cfg_meta meta;
  memset(cfg, 0, sizeof(*cfg));
  _i32 fh;
  _i32 r = sl_FsOpen(fn, FS_MODE_OPEN_READ, NULL, &fh);
  if (r != 0) return r;
  r = sl_FsRead(fh, 0, (_u8 *) &meta, sizeof(meta));
  if (r == sizeof(meta)) {
    memcpy(cfg, &meta.cfg, sizeof(*cfg));
    r = 0;
  } else {
    r = -1;
  }
  sl_FsClose(fh, NULL, NULL, 0);
  /*
   * The INVALID flag will not be set intentionally,
   * but will be set if it's just all 1s (e.g. empty sector).
   */
  if (cfg->flags & BOOT_F_INVALID) r = -10;
  return r;
}

static void cfg_fname(int idx, _u8 *fname) {
  int l = strlen(BOOT_CFG_FNAME_PREFIX);
  memcpy(fname, BOOT_CFG_FNAME_PREFIX, l);
  fname[l++] = '.';
  fname[l++] = '0' + idx;
  fname[l++] = '\0';
}

int read_boot_cfg(int idx, struct boot_cfg *cfg) {
  _u8 fname[15];
  cfg_fname(idx, fname);
  return read_cfg(fname, cfg);
}

int write_boot_cfg(const struct boot_cfg *cfg, int idx) {
  union boot_cfg_meta meta;
  memset(&meta, 0, sizeof(meta));
  memcpy(&meta.cfg, cfg, sizeof(*cfg));
  _u8 fname[15];
  cfg_fname(idx, fname);
  sl_FsDel(fname, 0);
  _i32 fh;
  _i32 r = sl_FsOpen(fname, FS_MODE_OPEN_CREATE(sizeof(meta), 0), NULL, &fh);
  if (r < 0) return r;
  r = sl_FsWrite(fh, 0, (_u8 *) &meta, sizeof(meta));
  if (r == sizeof(meta)) {
    r = 0;
  } else {
    r = -1;
  }
  sl_FsClose(fh, NULL, NULL, 0);
  return r;
}
