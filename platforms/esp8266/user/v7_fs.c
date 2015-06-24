/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "mem.h"
#include <errno.h>
#include "v7.h"

#ifndef V7_NO_FS

#include "./../spiffs/spiffs.h"

spiffs fs;

/* LOG_PAGE_SIZE have to be more than SPIFFS_OBJ_NAME_LEN */
#define LOG_PAGE_SIZE 256
#define FLASH_BLOCK_SIZE (4 * 1024)
#define FLASH_UNIT_SIZE 4

static u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
static u8_t spiffs_fds[16 * 4];

ICACHE_FLASH_ATTR static s32_t esp_spiffs_readwrite(u32_t addr, u32_t size,
                                                    u8 *p, int write) {
  /*
   * With proper configurarion spiffs never reads or writes more than
   * LOG_PAGE_SIZE
   */

  if (size > LOG_PAGE_SIZE) {
    os_printf("Invalid size provided to read/write (%d)\n\r", (int) size);
    return SPIFFS_ERR_NOT_CONFIGURED;
  }

  char tmp_buf[LOG_PAGE_SIZE + FLASH_UNIT_SIZE * 2];
  u32_t aligned_addr = addr & (-FLASH_UNIT_SIZE);
  u32_t aligned_size =
      ((size + (FLASH_UNIT_SIZE - 1)) & -FLASH_UNIT_SIZE) + FLASH_UNIT_SIZE;

  int res = spi_flash_read(aligned_addr, (u32_t *) tmp_buf, aligned_size);
  if (res != 0) {
    os_printf("spi_flash_read failed: %d (%d, %d)\n\r", res, (int) aligned_addr,
              (int) aligned_size);
    return res;
  }

  if (!write) {
    memcpy(p, tmp_buf + (addr - aligned_addr), size);
    return SPIFFS_OK;
  }

  memcpy(tmp_buf + (addr - aligned_addr), p, size);

  res = spi_flash_write(aligned_addr, (u32_t *) tmp_buf, aligned_size);
  if (res != 0) {
    os_printf("spi_flash_write failed: %d (%d, %d)\n\r", res,
              (int) aligned_addr, (int) aligned_size);
    return res;
  }

  return SPIFFS_OK;
}

ICACHE_FLASH_ATTR static s32_t esp_spiffs_read(u32_t addr, u32_t size,
                                               u8_t *dst) {
  if (addr % FLASH_UNIT_SIZE == 0 && size % FLASH_UNIT_SIZE == 0) {
    /*
     * Address & bufsize are aligned to 4, just reading
     * Since the most of operations are aligned there is no
     * reason always read into temporaty buffer
     */
    return spi_flash_read(addr, (u32_t *) dst, size);
  } else {
    return esp_spiffs_readwrite(addr, size, dst, 0);
  }
}

ICACHE_FLASH_ATTR static s32_t esp_spiffs_write(u32_t addr, u32_t size,
                                                u8_t *src) {
  if (addr % FLASH_UNIT_SIZE == 0 && size % FLASH_UNIT_SIZE == 0) {
    /*
     * Address & bufsize are aligned to 4, just reading
     * Since the most of operations are aligned there is no
     * reason always pre-read & over-write
     */
    return spi_flash_write(addr, (u32_t *) src, size);
  } else {
    return esp_spiffs_readwrite(addr, size, src, 1);
  }
}

ICACHE_FLASH_ATTR static s32_t esp_spiffs_erase(u32_t addr, u32_t size) {
  /*
   * With proper configurarion spiffs always
   * provides here sector address & sector size
   */
  if (size != FLASH_BLOCK_SIZE || addr % FLASH_BLOCK_SIZE != 0) {
    os_printf("Invalid size provided to esp_spiffs_erase (%d, %d)\n\r",
              (int) addr, (int) size);
    return SPIFFS_ERR_NOT_CONFIGURED;
  }

  return spi_flash_erase_sector(addr / FLASH_BLOCK_SIZE);
}

ICACHE_FLASH_ATTR int fs_init() {
  spiffs_config cfg;

#if !SPIFFS_SINGLETON
  /* FS_SIZE & FS_ADDR are provided via Makefile */
  cfg.phys_size = FS_SIZE;
  cfg.phys_addr = FS_ADDR;

  cfg.phys_erase_block = FLASH_BLOCK_SIZE;
  cfg.log_block_size = FLASH_BLOCK_SIZE;
  cfg.log_page_size = LOG_PAGE_SIZE;
#endif

  cfg.hal_read_f = esp_spiffs_read;
  cfg.hal_write_f = esp_spiffs_write;
  cfg.hal_erase_f = esp_spiffs_erase;

  return SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fds,
                      sizeof(spiffs_fds), 0, 0, 0);
}

/* Wrappers for V7 */

ICACHE_FLASH_ATTR void set_errno(int res) {
  if (res < 0) {
    errno = SPIFFS_errno(&fs);
  }
}

ICACHE_FLASH_ATTR void add_plus(char *ptr, int *open_mode) {
  if (*(ptr + 1) == '+') {
    *open_mode |= SPIFFS_RDWR;
  }
}

ICACHE_FLASH_ATTR int spiffs_fopen(const char *filename, const char *mode) {
  int open_mode = 0;
  char *ptr;
  int res;
  if ((ptr = strstr(mode, "w")) != NULL) {
    open_mode |= (SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_WRONLY);
    add_plus(ptr, &open_mode);
  }

  if ((ptr = strstr(mode, "r")) != NULL) {
    open_mode |= SPIFFS_RDONLY;
    add_plus(ptr, &open_mode);
  }

  if ((ptr = strstr(mode, "a")) != NULL) {
    open_mode |= (SPIFFS_APPEND | SPIFFS_WRONLY);
    add_plus(ptr, &open_mode);
  }

  res = SPIFFS_open(&fs, (char *) filename, open_mode, 0);
  set_errno(res);

  return res;
}

ICACHE_FLASH_ATTR size_t
spiffs_fread(void *ptr, size_t size, size_t count, int fd) {
  int res = SPIFFS_read(&fs, fd, ptr, size * count);
  set_errno(res);

  return res < 0 ? 0 : res;
}

ICACHE_FLASH_ATTR size_t
spiffs_fwrite(const void *ptr, size_t size, size_t count, int fd) {
  int res = SPIFFS_write(&fs, fd, (char *) ptr, size * count);
  set_errno(res);

  return res < 0 ? 0 : res;
}

ICACHE_FLASH_ATTR int spiffs_fclose(int fd) {
  SPIFFS_close(&fs, fd);
  return 0;
}

ICACHE_FLASH_ATTR int spiffs_rename(const char *oldname, const char *newname) {
  int res = SPIFFS_rename(&fs, (char *) oldname, (char *) newname);
  set_errno(res);

  return res;
}

ICACHE_FLASH_ATTR int spiffs_remove(const char *filename) {
  int res = SPIFFS_remove(&fs, (char *) filename);
  set_errno(res);

  return res;
}

ICACHE_FLASH_ATTR int v7_val_to_file(v7_val_t val) {
  return (int) v7_to_double(val);
}

ICACHE_FLASH_ATTR v7_val_t v7_file_to_val(int fd) {
  return v7_create_number(fd);
}

ICACHE_FLASH_ATTR int v7_is_file_type(v7_val_t val) {
  int res = v7_is_double(val);
  return res;
}

ICACHE_FLASH_ATTR int v7_get_file_size(int fd) {
  spiffs_stat stat;
  int res = SPIFFS_fstat(&fs, fd, &stat);
  set_errno(res);

  if (res < 0) {
    return res;
  }

  return stat.size;
}

ICACHE_FLASH_ATTR void spiffs_rewind(int fd) {
  int res = SPIFFS_lseek(&fs, fd, 0, SPIFFS_SEEK_SET);
  set_errno(res);
}

ICACHE_FLASH_ATTR int spiffs_ferror(int fd) {
  return SPIFFS_errno(&fs);
}

#endif
