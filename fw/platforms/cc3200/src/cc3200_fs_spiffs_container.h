/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_SPIFFS_CONTAINER_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_SPIFFS_CONTAINER_H_

#include "boot_cfg.h"
#include "cc3200_fs.h"
#include "cc3200_fs_spiffs.h"
#include "spiffs.h"

#include "common/platform.h"

typedef unsigned long long _u64;

struct mount_info {
  char *cpfx; /* Container filename prefix. */
  spiffs fs;
  _i32 fh;           /* FailFS file handle, or -1 if not open yet. */
  _u64 seq;          /* Sequence counter for the mounted container. */
  _u32 valid : 1;    /* 1 if this filesystem has been mounted. */
  _u32 cidx : 1;     /* Which of the two containers is currently mounted. */
  _u32 rw : 1;       /* 1 if the underlying fh is r/w. */
  double last_write; /* Last time container was written (systick) */
  /* SPIFFS work area and file descriptor space (malloced). */
  _u8 *work;
  _u8 *fds;
};

/* TI recommends rounding to nearest multiple of 4K - 512 bytes.
 * However, experiments have shown that you need to leave 1024 bytes at the end
 * otherwise additional 4K is allocated (compare AllocatedLen vs FileLen). */
#define FS_CONTAINER_SIZE(fs_size) (((((fs_size) >> 12) + 1) << 12) - 1024)

#define MAX_FS_CONTAINER_FNAME_LEN (MAX_FS_CONTAINER_PREFIX_LEN + 3)
void fs_container_fname(const char *cpfx, int cidx, _u8 *fname);

_i32 fs_create_container(const char *cpfx, int cidx, _u32 fs_size);

_i32 fs_delete_container(const char *cpfx, int cidx);

_i32 fs_delete_inactive_container(const char *cpfx);

_i32 fs_mount(const char *cpfx, struct mount_info *m);

_i32 fs_write_meta(_i32 fh, _u64 seq, _u32 fs_size, _u32 fs_block_size,
                   _u32 fs_page_size, _u32 fs_erase_size);

void fs_close_container(struct mount_info *m);

_i32 fs_umount(struct mount_info *m);

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_SPIFFS_CONTAINER_H_ */
