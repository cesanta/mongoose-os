/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_VFS_DEV_SLFS_CONTAINER_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_VFS_DEV_SLFS_CONTAINER_H_

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MGOS_DEV_TYPE_SLFS_CONTAINER "slfs_container"

/* TI recommends rounding to nearest multiple of 4K - 512 bytes.
 * However, experiments have shown that you need to leave 1024 bytes at the end
 * otherwise additional 4K is allocated (compare AllocatedLen vs FileLen). */
#define FS_CONTAINER_SIZE(fs_size) (((((fs_size) >> 12) + 1) << 12) - 1024)

bool cc3200_vfs_dev_slfs_container_register_type(void);

void cc3200_vfs_dev_slfs_container_fname(const char *cpfx, int cidx,
                                         uint8_t *fname);

bool cc3200_vfs_dev_slfs_container_write_meta(int fh, uint64_t seq,
                                              uint32_t fs_size,
                                              uint32_t fs_block_size,
                                              uint32_t fs_page_size,
                                              uint32_t fs_erase_size);

void cc3200_vfs_dev_slfs_container_delete_container(const char *cpfx, int cidx);

void cc3200_vfs_dev_slfs_container_delete_inactive_container(const char *cpfx);

void cc3200_vfs_dev_slfs_container_flush_all(void);

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_VFS_DEV_SLFS_CONTAINER_H_ */
