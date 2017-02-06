#include <stm32_sdk_hal.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "fs_data.h"
#include "stm32_spiffs.h"
#include "stm32_uart.h"
#include "common/spiffs/spiffs.h"
#include "common/cs_dbg.h"
#include "common/cs_dirent.h"
#include "fw/src/mgos_uart.h"

#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#define NUM_SYS_FD 3

#ifndef FS_LOG_PAGE_SIZE
#define FS_LOG_PAGE_SIZE 256
#endif

#ifndef FLASH_BLOCK_SIZE
#define FLASH_BLOCK_SIZE (4 * 1024)
#endif

#ifndef FLASH_ERASE_BLOCK_SIZE
/* STM32 have different sector sizes, 128 seems the max */
#define FLASH_ERASE_BLOCK_SIZE (128 * 1024)
#endif

#ifndef FS_MAX_OPEN_FILES
#define FS_MAX_OPEN_FILES 10
#endif

spiffs fs;

static s32_t stm32_spiffs_read(spiffs *fs, u32_t addr, u32_t size, u8_t *dst) {
  (void) fs;
  /* STM32 allows to read flash like memory */
  memcpy(dst, (void *) addr, size);
  return SPIFFS_OK;
}

static s32_t stm32_spiffs_write(spiffs *fs, u32_t addr, u32_t size, u8_t *src) {
  (void) fs;

  HAL_FLASH_Unlock();

  for (uint32_t i = 0; i < size; i++) {
    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, addr + i, *(src + i)) !=
        HAL_OK) {
      printf("Failed to write byte @%X\n", (int) (addr + i));
      break;
    }
  }

  HAL_FLASH_Lock();

  return SPIFFS_OK;
}

static s32_t stm32_spiffs_erase(spiffs *fs, u32_t addr, u32_t size) {
  (void) fs;
  int sec_no;

  /*
   * Accouring to all STM32 manuals I've read sectors 0-4 might have different
   * sizes and addresses, but sector 5 is always @0x08020000 and
   * sectors 5...max are 128kb sized. Our FS located somewhere after sector 5
   */

  /* all we know is a start address of FLASH_SECTOR_5 */
  sec_no = addr - 0x08020000;
  /* so, get count of required sector from FLASH_SECTOR_5 */
  sec_no /= (128 * 1024);
  /* ... and calculate actual sector number */
  sec_no += FLASH_SECTOR_5;

  if (!IS_FLASH_SECTOR(sec_no)) {
    LOG(LL_ERROR, ("Wrong erase address: %X", (int) addr));
    return SPIFFS_ERR_ERASE_FAIL;
  }

  HAL_FLASH_Unlock();
  FLASH_Erase_Sector(sec_no, VOLTAGE_RANGE_3);
  HAL_FLASH_Lock();

  return SPIFFS_OK;
}

int stm32_spiffs_init() {
  static uint8_t spiffs_work_buf[FS_LOG_PAGE_SIZE * 2];
  static uint8_t spiffs_fds[SPIFFS_OBJ_NAME_LEN * FS_MAX_OPEN_FILES];

  spiffs_config cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.phys_addr = (uint32_t) fs_bin;
  cfg.phys_size = FS_SIZE;
  cfg.log_page_size = FS_LOG_PAGE_SIZE;
  cfg.log_block_size = FLASH_BLOCK_SIZE;
  cfg.phys_erase_block = FLASH_ERASE_BLOCK_SIZE;

  cfg.hal_read_f = stm32_spiffs_read;
  cfg.hal_write_f = stm32_spiffs_write;
  cfg.hal_erase_f = stm32_spiffs_erase;

  if (SPIFFS_mount(&fs, &cfg, spiffs_work_buf, spiffs_fds, sizeof(spiffs_fds),
                   0, 0, 0) != SPIFFS_OK) {
    LOG(LL_ERROR, ("SPIFFS_mount failed: %d", (int) SPIFFS_errno(&fs)));
    return SPIFFS_errno(&fs);
  }

  LOG(LL_DEBUG, ("FS Mounted successfully"));
  return 0;
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

static void set_errno(int res) {
  if (res < 0) {
    errno = SPIFFS_errno(&fs);
    LOG(LL_DEBUG, ("spiffs error: %d\n", errno));
  }
}

static char *get_fixed_filename(const char *filename) {
  /* spiffs doesn't support directories, not even the trivial ./something */
  while (*filename != '\0' && (*filename == '/' || *filename == '.')) {
    filename++;
  }

  /*
   * all SPIFFs functions doesn't work with const char *, so
   * removing const here
   */
  return (char *) filename;
}

int _open_r(struct _reent *r, const char *filename, int flags, int mode) {
  spiffs_mode sm = 0;
  int res;
  int rw = (flags & 3);
  (void) r;
  (void) mode;
  if (rw == O_RDONLY || rw == O_RDWR) sm |= SPIFFS_RDONLY;
  if (rw == O_WRONLY || rw == O_RDWR) sm |= SPIFFS_WRONLY | SPIFFS_CREAT;
  if (flags & O_CREAT) sm |= SPIFFS_CREAT;
  if (flags & O_TRUNC) sm |= SPIFFS_TRUNC;
  if (flags & O_APPEND) sm |= SPIFFS_APPEND | SPIFFS_CREAT;

  /* Supported in newer versions of SPIFFS. */
  /* if (flags && O_EXCL) sm |= SPIFFS_EXCL; */
  /* if (flags && O_DIRECT) sm |= SPIFFS_DIRECT; */

  res = SPIFFS_open(&fs, get_fixed_filename(filename), sm, 0);
  if (res >= 0) {
    res += NUM_SYS_FD;
  }
  set_errno(res);
  return res;
}

_ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t len) {
  ssize_t res;
  (void) r;
  if (fd < NUM_SYS_FD) {
    res = -1;
  } else {
    res = SPIFFS_read(&fs, fd - NUM_SYS_FD, buf, len);
  }
  set_errno(res);
  return res;
}

_ssize_t _write_r(struct _reent *r, int fd, void *buf, size_t len) {
  int uart_no = -1;

  if (fd == STDOUT_FILENO) {
    uart_no = stm32_get_stdout_uart_no();
  } else if (fd == STDERR_FILENO) {
    uart_no = stm32_get_stderr_uart_no();
  }

  if (uart_no >= 0) {
    mgos_uart_write(uart_no, buf, len);
    return len;
  }

  int res = SPIFFS_write(&fs, fd - NUM_SYS_FD, (char *) buf, len);
  set_errno(res);
  return res;
}

_off_t _lseek_r(struct _reent *r, int fd, _off_t where, int whence) {
  ssize_t res;
  (void) r;
  if (fd < NUM_SYS_FD) {
    res = -1;
  } else {
    res = SPIFFS_lseek(&fs, fd - NUM_SYS_FD, where, whence);
  }
  set_errno(res);
  return res;
}

int _close_r(struct _reent *r, int fd) {
  (void) r;
  if (fd < NUM_SYS_FD) {
    return -1;
  }
  SPIFFS_close(&fs, fd - NUM_SYS_FD);
  return 0;
}

int _rename_r(struct _reent *r, const char *from, const char *to) {
  /*
   * POSIX rename requires that in case "to" exists, it be atomically replaced
   * with "from". The atomic part we can't do, but at least we can do replace.
   */
  int res;
  {
    spiffs_stat ss;
    res = SPIFFS_stat(&fs, (char *) to, &ss);
    if (res == 0) {
      SPIFFS_remove(&fs, (char *) to);
    }
  }
  res = SPIFFS_rename(&fs, get_fixed_filename(from), get_fixed_filename(to));
  set_errno(res);
  return res;
}

int _unlink_r(struct _reent *r, const char *filename) {
  int res = SPIFFS_remove(&fs, get_fixed_filename(filename));
  (void) r;
  set_errno(res);

  return res;
}

int _fstat_r(struct _reent *r, int fd, struct stat *s) {
  int res;
  spiffs_stat ss;
  (void) r;
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
  s->st_mode = S_IFREG | 0666;
  s->st_nlink = 1;
  s->st_size = ss.size;
  return 0;
}

int _stat_r(struct _reent *r, const char *path, struct stat *s) {
  int ret, fd;
  (void) r;

  /*
   * spiffs has no directories, simulating statting root directory;
   * required for mg_send_http_file.
   */
  if ((strcmp(path, ".") == 0) || (strcmp(path, "/") == 0) ||
      (strcmp(path, "./") == 0)) {
    memset(s, 0, sizeof(*s));
    s->st_mode = S_IFDIR;
    return 0;
  }

  fd = _open_r(NULL, path, O_RDONLY, 0);
  if (fd == -1) return -1;
  ret = _fstat_r(NULL, fd, s);
  _close_r(NULL, fd);
  return ret;
}

struct spiffs_dir {
  DIR dir;
  spiffs_DIR sdh;
  struct spiffs_dirent sde;
  struct dirent de;
};

DIR *opendir(const char *dir_name) {
  struct spiffs_dir *sd = NULL;

  if (dir_name == NULL) {
    errno = EINVAL;
    return NULL;
  }

  if ((sd = (struct spiffs_dir *) calloc(1, sizeof(*sd))) == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  if (SPIFFS_opendir(&fs, dir_name, &sd->sdh) == NULL) {
    free(sd);
    sd = NULL;
    errno = EINVAL;
  }

  return (DIR *) sd;
}

struct dirent *readdir(DIR *dir) {
  struct spiffs_dir *sd = (struct spiffs_dir *) dir;
  if (SPIFFS_readdir(&sd->sdh, &sd->sde) == SPIFFS_OK) {
    errno = EBADF;
    return NULL;
  }
  sd->de.d_ino = sd->sde.obj_id;
  memcpy(sd->de.d_name, sd->sde.name, SPIFFS_OBJ_NAME_LEN);
  return &sd->de;
}

int closedir(DIR *dir) {
  struct spiffs_dir *sd = (struct spiffs_dir *) dir;
  if (dir != NULL) {
    SPIFFS_closedir(&sd->sdh);
    free(dir);
  }
  return 0;
}
