#ifndef CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_VFS_DEV_SLFS_CONTAINER_META_H_
#define CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_VFS_DEV_SLFS_CONTAINER_META_H_

#include <stdint.h>

#define MAX_FS_CONTAINER_PREFIX_LEN 50
#define MAX_FS_CONTAINER_FNAME_LEN (MAX_FS_CONTAINER_PREFIX_LEN + 3)

struct fs_container_info {
  uint64_t seq;
  uint32_t fs_size;
  /* These are no longer actually used, left for backward compat. */
  uint32_t fs_block_size;
  uint32_t fs_page_size;
  uint32_t fs_erase_size;
} info;

#define FS_INITIAL_SEQ (~(0ULL) - 1ULL)

union fs_container_meta {
  struct fs_container_info info;
  uint8_t padding[64];
};

#endif /* CS_FW_PLATFORMS_CC32XX_SRC_CC32XX_VFS_DEV_SLFS_CONTAINER_META_H_ */
