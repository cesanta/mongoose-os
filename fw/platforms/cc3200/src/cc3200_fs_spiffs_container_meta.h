#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_SPIFFS_CONTAINER_META_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_SPIFFS_CONTAINER_META_H_

#include <inttypes.h>

struct fs_container_info {
  uint64_t seq;
  uint32_t fs_size;
  uint32_t fs_block_size;
  uint32_t fs_page_size;
  uint32_t fs_erase_size;
} info;

#define FS_INITIAL_SEQ (~(0ULL) - 1ULL)

union fs_container_meta {
  struct fs_container_info info;
  uint8_t padding[64];
};

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_SPIFFS_CONTAINER_META_H_ */
