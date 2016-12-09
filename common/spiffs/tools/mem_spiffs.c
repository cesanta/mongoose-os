#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>

#include <spiffs.h>

#define LOG_PAGE_SIZE 256
#define FLASH_BLOCK_SIZE (4 * 1024)

spiffs fs;
u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
u8_t spiffs_fds[32 * 4];

char *image; /* in memory flash image */
size_t image_size;

s32_t mem_spiffs_read(spiffs *fs, u32_t addr, u32_t size, u8_t *dst) {
  memcpy(dst, image + addr, size);
  return SPIFFS_OK;
}

s32_t mem_spiffs_write(spiffs *fs, u32_t addr, u32_t size, u8_t *src) {
  memcpy(image + addr, src, size);
  return SPIFFS_OK;
}

s32_t mem_spiffs_erase(spiffs *fs, u32_t addr, u32_t size) {
  memset(image + addr, 0xff, size);
  return SPIFFS_OK;
}

int mem_spiffs_mount(void) {
  spiffs_config cfg;

  cfg.phys_size = image_size;
  cfg.phys_addr = 0;

  cfg.phys_erase_block = FLASH_BLOCK_SIZE;
  cfg.log_block_size = FLASH_BLOCK_SIZE;
  cfg.log_page_size = LOG_PAGE_SIZE;

  cfg.hal_read_f = mem_spiffs_read;
  cfg.hal_write_f = mem_spiffs_write;
  cfg.hal_erase_f = mem_spiffs_erase;

  return SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fds,
                      sizeof(spiffs_fds), 0, 0, 0);
}
