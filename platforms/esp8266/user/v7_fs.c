/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#include <ets_sys.h>
#include <errno.h>
#include <fcntl.h>
#include <v7.h>

#ifndef RTOS_SDK

#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "mem.h"
#include <errno.h>
#include <fcntl.h>

#else

#include <c_types.h>
#include <spi_flash.h>

#endif /* RTOS_SDK */

#ifndef V7_NO_FS

#include "spiffs/spiffs.h"
#include "esp_uart.h"

/*
 * number of file descriptors reserved for system.
 * SPIFFS currently returns file descriptors that
 * clash with "system" fds like stdout and stderr.
 * Here we remap all spiffs fds by adding/subtracting NUM_SYS_FD
 */
#define NUM_SYS_FD 3

spiffs fs;

/* LOG_PAGE_SIZE have to be more than SPIFFS_OBJ_NAME_LEN */
#define LOG_PAGE_SIZE 256
#define FLASH_BLOCK_SIZE (4 * 1024)
#define FLASH_UNIT_SIZE 4

#ifndef FS_MAX_OPEN_FILES
#define FS_MAX_OPEN_FILES 10
#endif

static u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
static u8_t spiffs_fds[32 * FS_MAX_OPEN_FILES];

int spiffs_get_memory_usage() {
  return sizeof(spiffs_work_buf) + sizeof(spiffs_fds);
}

static s32_t esp_spiffs_readwrite(u32_t addr, u32_t size, u8 *p, int write) {
  /*
   * With proper configurarion spiffs never reads or writes more than
   * LOG_PAGE_SIZE
   */

  if (size > LOG_PAGE_SIZE) {
    printf("Invalid size provided to read/write (%d)\n\r", (int) size);
    return SPIFFS_ERR_NOT_CONFIGURED;
  }

  char tmp_buf[LOG_PAGE_SIZE + FLASH_UNIT_SIZE * 2];
  u32_t aligned_addr = addr & (-FLASH_UNIT_SIZE);
  u32_t aligned_size =
      ((size + (FLASH_UNIT_SIZE - 1)) & -FLASH_UNIT_SIZE) + FLASH_UNIT_SIZE;

  int res = spi_flash_read(aligned_addr, (u32_t *) tmp_buf, aligned_size);
  if (res != 0) {
    printf("spi_flash_read failed: %d (%d, %d)\n\r", res, (int) aligned_addr,
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
    printf("spi_flash_write failed: %d (%d, %d)\n\r", res, (int) aligned_addr,
           (int) aligned_size);
    return res;
  }

  return SPIFFS_OK;
}

static s32_t esp_spiffs_read(u32_t addr, u32_t size, u8_t *dst) {
  if (0 && addr % FLASH_UNIT_SIZE == 0 && size % FLASH_UNIT_SIZE == 0) {
    /*
     * For unknown reason spi_flash_read/write
     * hangs from time to time if size is small (< 8)
     * and address is not aligned to 0xFF
     * TODO(alashkin): understand why and remove `0 &&` from `if`
     */
    return spi_flash_read(addr, (u32_t *) dst, size);
  } else {
    return esp_spiffs_readwrite(addr, size, dst, 0);
  }
}

static s32_t esp_spiffs_write(u32_t addr, u32_t size, u8_t *src) {
  if (0 && addr % FLASH_UNIT_SIZE == 0 && size % FLASH_UNIT_SIZE == 0) {
    /*
     * For unknown reason spi_flash_read/write
     * hangs from time to time if size is small (< 8)
     * and address is not aligned to 0xFF
     * TODO(alashkin): understand why and remove `0 &&` from `if`
     */
    return spi_flash_write(addr, (u32_t *) src, size);
  } else {
    return esp_spiffs_readwrite(addr, size, src, 1);
  }
}

static s32_t esp_spiffs_erase(u32_t addr, u32_t size) {
  /*
   * With proper configurarion spiffs always
   * provides here sector address & sector size
   */
  if (size != FLASH_BLOCK_SIZE || addr % FLASH_BLOCK_SIZE != 0) {
    printf("Invalid size provided to esp_spiffs_erase (%d, %d)\n\r", (int) addr,
           (int) size);
    return SPIFFS_ERR_NOT_CONFIGURED;
  }

  return spi_flash_erase_sector(addr / FLASH_BLOCK_SIZE);
}

int fs_init() {
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

void set_errno(int res) {
  if (res < 0) {
    errno = SPIFFS_errno(&fs);
  }
}

void add_plus(char *ptr, int *open_mode) {
  if (*(ptr + 1) == '+') {
    *open_mode |= SPIFFS_RDWR;
  }
}

int _open_r(struct _reent *r, const char *filename, int flags, int mode) {
  spiffs_mode sm = 0;
  int res;
  int rw = (flags & 3);
  if (rw == O_RDONLY || rw == O_RDWR) sm |= SPIFFS_RDONLY;
  if (rw == O_WRONLY || rw == O_RDWR) sm |= SPIFFS_WRONLY;
  if (flags & O_CREAT) sm |= SPIFFS_CREAT;
  if (flags & O_TRUNC) sm |= SPIFFS_TRUNC;
  if (flags & O_APPEND) sm |= SPIFFS_APPEND;
  /* Supported in newer versions of SPIFFS. */
  /* if (flags && O_EXCL) sm |= SPIFFS_EXCL; */
  /* if (flags && O_DIRECT) sm |= SPIFFS_DIRECT; */

  res = SPIFFS_open(&fs, (char *) filename, sm, 0);
  if (res >= 0) {
    res += NUM_SYS_FD;
  }
  set_errno(res);
  return res;
}

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t len) {
  ssize_t res;
  if (fd < NUM_SYS_FD) {
    res = -1;
  } else {
    res = SPIFFS_read(&fs, fd - NUM_SYS_FD, buf, len);
  }
  set_errno(res);
  return res;
}

_ssize_t _write_r(struct _reent *r, int fd, void *buf, size_t len) {
  if (fd < NUM_SYS_FD) {
    uart_write(fd, buf, len);
    return len;
  }

  int res = SPIFFS_write(&fs, fd - NUM_SYS_FD, (char *) buf, len);
  set_errno(res);
  return res;
}

_off_t _lseek_r(struct _reent *r, int fd, _off_t where, int whence) {
  ssize_t res;
  if (fd < NUM_SYS_FD) {
    res = -1;
  } else {
    res = SPIFFS_lseek(&fs, fd - NUM_SYS_FD, where, whence);
  }
  set_errno(res);
  return res;
}

int _close_r(struct _reent *r, int fd) {
  if (fd < NUM_SYS_FD) {
    return -1;
  }
  SPIFFS_close(&fs, fd - NUM_SYS_FD);
  return 0;
}

int _rename_r(struct _reent *r, const char *from, const char *to) {
  int res = SPIFFS_rename(&fs, (char *) from, (char *) to);
  set_errno(res);

  return res;
}

int _unlink_r(struct _reent *r, const char *filename) {
  int res = SPIFFS_remove(&fs, (char *) filename);
  set_errno(res);

  return res;
}

int _fstat_r(struct _reent *r, int fd, struct stat *s) {
  int res;
  spiffs_stat ss;
  memset(s, 0, sizeof(*s));
  if (fd < NUM_SYS_FD) {
    s->st_ino = fd;
    s->st_rdev = fd;
    s->st_mode = S_IFCHR | 0666;
    return 0;
  }
  res = SPIFFS_fstat(&fs, fd - NUM_SYS_FD, &ss);
  set_errno(res);
  if (res < 0) return res;
  s->st_ino = ss.obj_id;
  s->st_mode = 0666;
  s->st_nlink = 1;
  s->st_size = ss.size;
  return 0;
}

int v7_val_to_file(v7_val_t val) {
  return (int) v7_to_number(val);
}

v7_val_t v7_file_to_val(int fd) {
  return v7_create_number(fd);
}

int v7_is_file_type(v7_val_t val) {
  int res = v7_is_number(val);
  return res;
}

#endif
