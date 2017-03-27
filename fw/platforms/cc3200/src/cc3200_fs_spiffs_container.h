/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_SPIFFS_CONTAINER_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_SPIFFS_CONTAINER_H_

#include <stdint.h>

#include "cc3200_fs.h"

#include "common/platform.h"

#include <osi.h>

#include <spiffs.h>

typedef uint64_t _u64;

struct mount_info {
  OsiLockObj_t lock; /* Lock guarding access to all the members below. */
  char *cpfx;        /* Container filename prefix. */
  _i32 fh;           /* SLFS file handle, or -1 if not open yet. */
  struct spiffs_t fs;
  uint64_t seq;      /* Sequence counter for the mounted container. */
  uint32_t cidx : 1; /* Which of the two containers is currently mounted. */
  uint32_t rw : 1;   /* 1 if the underlying fh is r/w. */
  double last_write; /* Last time container was written (systick) */
  /* SPIFFS work area and file descriptor space (malloced). */
  uint8_t *work;
  uint8_t *fds;
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

void fs_lock(struct mount_info *m);
void fs_unlock(struct mount_info *m);

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_SPIFFS_CONTAINER_H_ */
