/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#include <dirent.h>
#include <fcntl.h>

#include "fw/src/mgos_features.h"

#include "c_types.h"
#include "spi_flash.h"

#include "common/cs_dbg.h"

#include "common/spiffs/spiffs.h"
#include "common/spiffs/spiffs_nucleus.h"
#include "common/spiffs/spiffs_vfs.h"
#include "spiffs_config.h"

#include "esp_fs.h"
#include "fw/src/mgos_uart.h"
#include "mongoose/mongoose.h"

#include <sys/mman.h>

/*
 * number of file descriptors reserved for system.
 * SPIFFS currently returns file descriptors that
 * clash with "system" fds like stdout and stderr.
 * Here we remap all spiffs fds by adding/subtracting NUM_SYS_FD
 */
#define NUM_SYS_FD 3

static spiffs fs;

#define FLASH_BLOCK_SIZE (4 * 1024)
#define FLASH_UNIT_SIZE 4

#ifndef FS_MAX_OPEN_FILES
#define FS_MAX_OPEN_FILES 10
#endif

static u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
static u8_t spiffs_fds[32 * FS_MAX_OPEN_FILES];

static int8_t s_stdout_uart = MGOS_DEBUG_UART;
static int8_t s_stderr_uart = MGOS_DEBUG_UART;

/* For cs_dirent.c functions */
spiffs *cs_spiffs_get_fs(void) {
  return &fs;
}

static s32_t esp_spiffs_readwrite(u32_t addr, u32_t size, u8 *p, int write) {
  /*
   * With proper configurarion spiffs never reads or writes more than
   * LOG_PAGE_SIZE
   */

  if (size > LOG_PAGE_SIZE) {
    LOG(LL_ERROR, ("Invalid size provided to read/write (%d)", (int) size));
    return SPIFFS_ERR_NOT_CONFIGURED;
  }

  char tmp_buf[LOG_PAGE_SIZE + FLASH_UNIT_SIZE * 2];
  u32_t aligned_addr = addr & (-FLASH_UNIT_SIZE);
  u32_t aligned_size =
      ((size + (FLASH_UNIT_SIZE - 1)) & -FLASH_UNIT_SIZE) + FLASH_UNIT_SIZE;

  int res = spi_flash_read(aligned_addr, (u32_t *) tmp_buf, aligned_size);
  if (res != 0) {
    LOG(LL_ERROR, ("spi_flash_read failed: %d (%d, %d)", res,
                   (int) aligned_addr, (int) aligned_size));
    return res;
  }

  if (!write) {
    memcpy(p, tmp_buf + (addr - aligned_addr), size);
    return SPIFFS_OK;
  }

  memcpy(tmp_buf + (addr - aligned_addr), p, size);

  res = spi_flash_write(aligned_addr, (u32_t *) tmp_buf, aligned_size);
  if (res != 0) {
    LOG(LL_ERROR, ("spi_flash_write failed: %d (%d, %d)", res,
                   (int) aligned_addr, (int) aligned_size));
    return res;
  }

  return SPIFFS_OK;
}

static s32_t esp_spiffs_read(spiffs *fs, u32_t addr, u32_t size, u8_t *dst) {
  return esp_spiffs_readwrite(addr, size, dst, 0);
  (void) fs;
}

static s32_t esp_spiffs_write(spiffs *fs, u32_t addr, u32_t size, u8_t *src) {
  return esp_spiffs_readwrite(addr, size, src, 1);
  (void) fs;
}

static s32_t esp_spiffs_erase(spiffs *fs, u32_t addr, u32_t size) {
  /*
   * With proper configurarion spiffs always
   * provides here sector address & sector size
   */
  if (size != FLASH_BLOCK_SIZE || addr % FLASH_BLOCK_SIZE != 0) {
    LOG(LL_ERROR, ("Invalid size provided to esp_spiffs_erase (%d, %d)",
                   (int) addr, (int) size));
    return SPIFFS_ERR_NOT_CONFIGURED;
  }
  (void) fs;
  return spi_flash_erase_sector(addr / FLASH_BLOCK_SIZE);
}

int fs_mount(spiffs *spf, uint32_t addr, uint32_t size, uint8_t *workbuf,
             uint8_t *fds, size_t fds_size) {
  LOG(LL_INFO, ("Mounting FS: %d @ 0x%x", size, addr));
  spiffs_config cfg;

  cfg.phys_addr = addr;
  cfg.phys_size = size;

  cfg.phys_erase_block = FLASH_BLOCK_SIZE;
  cfg.log_block_size = FLASH_BLOCK_SIZE;
  cfg.log_page_size = LOG_PAGE_SIZE;

  cfg.hal_read_f = esp_spiffs_read;
  cfg.hal_write_f = esp_spiffs_write;
  cfg.hal_erase_f = esp_spiffs_erase;

  if (SPIFFS_mount(spf, &cfg, workbuf, fds, fds_size, 0, 0, 0) != SPIFFS_OK) {
    LOG(LL_ERROR, ("SPIFFS_mount failed: %d", SPIFFS_errno(spf)));
    return SPIFFS_errno(spf);
  }

  return 0;
}

int fs_init(uint32_t addr, uint32_t size) {
  return fs_mount(&fs, addr, size, spiffs_work_buf, spiffs_fds,
                  sizeof(spiffs_fds));
}

void fs_umount(void) {
  SPIFFS_unmount(&fs);
}

size_t mgos_get_fs_size(void) {
  u32_t total, used;
  if (SPIFFS_info(&fs, &total, &used) != SPIFFS_OK) return 0;
  return total;
}

size_t mgos_get_free_fs_size(void) {
  u32_t total, used;
  if (SPIFFS_info(&fs, &total, &used) != SPIFFS_OK) return 0;
  return total - used;
}

int _open_r(struct _reent *r, const char *filename, int flags, int mode) {
  int res = spiffs_vfs_open(&fs, filename, flags, mode);
  if (res >= 0) {
    res += NUM_SYS_FD;
  }
  (void) r;
  return res;
}

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t len) {
  ssize_t res;
  (void) r;
  if (fd < NUM_SYS_FD) {
    res = -1;
    errno = EBADF;
  } else {
    res = spiffs_vfs_read(&fs, fd - NUM_SYS_FD, buf, len);
  }
  (void) r;
  return res;
}

_ssize_t _write_r(struct _reent *r, int fd, void *buf, size_t len) {
  (void) r;
  if (fd < NUM_SYS_FD) {
    int uart_no = -1;
    if (fd == 1) {
      uart_no = s_stdout_uart;
    } else if (fd == 2) {
      uart_no = s_stderr_uart;
    } else if (fd == 0) {
      errno = EBADF;
      len = -1;
    }
    if (uart_no >= 0) len = mgos_uart_write(uart_no, buf, len);
    return len;
  }

  return spiffs_vfs_write(&fs, fd - NUM_SYS_FD, buf, len);
}

_off_t _lseek_r(struct _reent *r, int fd, _off_t where, int whence) {
  ssize_t res;
  (void) r;
  if (fd < NUM_SYS_FD) {
    res = -1;
    errno = EBADF;
  } else {
    res = spiffs_vfs_lseek(&fs, fd - NUM_SYS_FD, where, whence);
  }
  return res;
}

int _close_r(struct _reent *r, int fd) {
  ssize_t res;
  (void) r;
  if (fd < NUM_SYS_FD) {
    res = -1;
    errno = EBADF;
  } else {
    res = spiffs_vfs_close(&fs, fd - NUM_SYS_FD);
  }
  return res;
}

int _rename_r(struct _reent *r, const char *from, const char *to) {
  (void) r;
  return spiffs_vfs_rename(&fs, from, to);
}

int _unlink_r(struct _reent *r, const char *filename) {
  (void) r;
  return spiffs_vfs_unlink(&fs, filename);
}

int _fstat_r(struct _reent *r, int fd, struct stat *s) {
  (void) r;
  memset(s, 0, sizeof(*s));
  if (fd < NUM_SYS_FD) {
    s->st_ino = fd;
    s->st_rdev = fd;
    s->st_mode = S_IFCHR | 0666;
    return 0;
  }
  return spiffs_vfs_fstat(&fs, fd - NUM_SYS_FD, s);
}

int _stat_r(struct _reent *r, const char *path, struct stat *s) {
  (void) r;
  return spiffs_vfs_stat(&fs, path, s);
}

DIR *opendir(const char *path) {
  return spiffs_vfs_opendir(&fs, path);
}

struct dirent *readdir(DIR *dir) {
  return spiffs_vfs_readdir(&fs, dir);
}

int closedir(DIR *dir) {
  return spiffs_vfs_closedir(&fs, dir);
}

void fs_flush_stderr(void) {
  if (s_stderr_uart >= 0) mgos_uart_flush(s_stderr_uart);
}

enum mgos_init_result mgos_set_stdout_uart(int uart_no) {
  enum mgos_init_result r = mgos_init_debug_uart(uart_no);
  if (r == MGOS_INIT_OK) {
    s_stdout_uart = uart_no;
  }
  return r;
}

enum mgos_init_result mgos_set_stderr_uart(int uart_no) {
  enum mgos_init_result r = mgos_init_debug_uart(uart_no);
  if (r == MGOS_INIT_OK) {
    s_stderr_uart = uart_no;
  }
  return r;
}

enum mgos_init_result esp_console_init() {
  return mgos_init_debug_uart(MGOS_DEBUG_UART);
}
