#ifndef ESP_FS_INCLUDED
#define ESP_FS_INCLUDED

#ifndef SJ_MMAP_SLOTS
#define SJ_MMAP_SLOTS 16
#endif

#include "spiffs/spiffs.h"

/* LOG_PAGE_SIZE have to be more than SPIFFS_OBJ_NAME_LEN */
#define LOG_PAGE_SIZE 256
#define SPIFFS_PAGE_HEADER_SIZE 5
#define SPIFFS_PAGE_DATA_SIZE ((LOG_PAGE_SIZE) - (SPIFFS_PAGE_HEADER_SIZE))
#define MMAP_BASE ((void *) 0x10000000)
#define MMAP_END ((void *) 0x20000000)
#define MMAP_DESC_BITS 24
#define FLASH_BASE 0x40200000

#define MMAP_DESC_FROM_ADDR(addr) \
  (&mmap_descs[(((uintptr_t) addr) >> MMAP_DESC_BITS) & 0xF])
#define MMAP_ADDR_FROM_DESC(desc) \
  ((void *) ((uintptr_t) MMAP_BASE | ((desc - mmap_descs) << MMAP_DESC_BITS)))

struct mmap_desc {
  void *base;
  uint32_t pages;
  uint32_t *blocks; /* pages long */
};

extern struct mmap_desc mmap_descs[SJ_MMAP_SLOTS];

int fs_init(uint32_t addr, uint32_t size);
int fs_mount(spiffs *spf, uint32_t addr, uint32_t size, uint8_t *workbuf,
             uint8_t *fds, size_t fds_size);

#endif /* V7_FS_INCLUDED */
