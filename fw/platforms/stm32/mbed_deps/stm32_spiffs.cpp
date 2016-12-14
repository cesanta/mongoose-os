#include "mbed.h"
#include "fs_data.h"
#include "stm32_spiffs.h"
#include "common/spiffs/spiffs.h"
#include "common/cs_dbg.h"

#include <errno.h>

#ifndef FS_LOG_PAGE_SIZE
#define FS_LOG_PAGE_SIZE 256
#endif

#ifndef FLASH_BLOCK_SIZE
#define FLASH_BLOCK_SIZE (4 * 1024)
#endif

#ifndef FS_MAX_OPEN_FILES
#define FS_MAX_OPEN_FILES 10
#endif

#define NUM_SYS_FD 3

static spiffs s_fs;

static s32_t stm32_spiffs_read(spiffs *fs, u32_t addr, u32_t size, u8_t *dst) {
  (void) fs;
  /* STM32 allows to read flash like memory */
  memcpy(dst, (void*)addr, size);
  return SPIFFS_OK;
}

static s32_t stm32_spiffs_write(spiffs *fs, u32_t addr, u32_t size, u8_t *src) {
  (void) fs;
  (void) addr;
  (void) src;

  printf("TODO(alex): implement stm32_spiffs_write\n");
  return SPIFFS_ERR_NOT_WRITABLE;
}

static s32_t stm32_spiffs_erase(spiffs *fs, u32_t addr, u32_t size) {
  (void) fs;
  (void) addr;
  (void) size;

  printf("TODO(alex): implement stm32_spiffs_erase\n");
  return SPIFFS_ERR_NOT_WRITABLE;
}

int miot_stm32_init_spiffs_init() {
  static uint8_t spiffs_work_buf[FS_LOG_PAGE_SIZE * 2];
  static uint8_t spiffs_fds[SPIFFS_OBJ_NAME_LEN * FS_MAX_OPEN_FILES];

  spiffs_config cfg;
  memset(&cfg, 0, sizeof(cfg));
  cfg.phys_addr = (uint32_t)fs_bin;
  cfg.phys_size = FS_SIZE;
  cfg.log_page_size = FS_LOG_PAGE_SIZE;
  cfg.log_block_size = FLASH_BLOCK_SIZE;

  cfg.hal_read_f = stm32_spiffs_read;
  cfg.hal_write_f = stm32_spiffs_write;
  cfg.hal_erase_f = stm32_spiffs_erase;

  if (SPIFFS_mount(&s_fs, &cfg, spiffs_work_buf, spiffs_fds,
                    sizeof(spiffs_fds), 0, 0, 0) != SPIFFS_OK) {
    LOG(LL_ERROR, ("SPIFFS_mount failed: %d", (int)SPIFFS_errno(&s_fs)));
    return SPIFFS_errno(&s_fs);
  }

  LOG(LL_DEBUG, ("FS Mounted successfully"));
  return 0;
}

static void set_errno(int res) {
  if (res < 0) {
    errno = SPIFFS_errno(&s_fs);
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

extern "C" int _open_r(struct _reent *r, const char *filename, int flags, int mode) {
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

  res = SPIFFS_open(&s_fs, get_fixed_filename(filename), sm, 0);
  if (res >= 0) {
    res += NUM_SYS_FD;
  }
  set_errno(res);
  return res;
}

extern "C" _ssize_t _read_r(struct _reent *r, int fd, void *buf, size_t len) {
  ssize_t res;
  (void) r;
  if (fd < NUM_SYS_FD) {
    res = -1;
  } else {
    res = SPIFFS_read(&s_fs, fd - NUM_SYS_FD, buf, len);
  }
  set_errno(res);
  return res;
}

/*
 * TODO(alashkin): enabling of this block breaks default
 * implementation of printf & Co, fix this and enable
 */

#if 0
extern "C" _ssize_t _write_r(struct _reent *r, int fd, void *buf, size_t len) {
  (void) r;
  if (fd < NUM_SYS_FD) {
    Serial p(SERIAL_TX, SERIAL_RX);
    p.puts("Hello, world!\n");
  }

  int res = SPIFFS_write(&s_fs, fd - NUM_SYS_FD, (char *) buf, len);
  set_errno(res);
  return res;
}
#endif

extern "C" _off_t _lseek_r(struct _reent *r, int fd, _off_t where, int whence) {
  ssize_t res;
  (void) r;
  if (fd < NUM_SYS_FD) {
    res = -1;
  } else {
    res = SPIFFS_lseek(&s_fs, fd - NUM_SYS_FD, where, whence);
  }
  set_errno(res);
  return res;
}

int _close_r(struct _reent *r, int fd) {
  (void) r;
  if (fd < NUM_SYS_FD) {
    return -1;
  }
  SPIFFS_close(&s_fs, fd - NUM_SYS_FD);
  return 0;
}

extern "C" int _rename_r(struct _reent *r, const char *from, const char *to) {
  /*
   * POSIX rename requires that in case "to" exists, it be atomically replaced
   * with "from". The atomic part we can't do, but at least we can do replace.
   */
  int res;
  {
    spiffs_stat ss;
    res = SPIFFS_stat(&s_fs, (char *) to, &ss);
    if (res == 0) {
      SPIFFS_remove(&s_fs, (char *) to);
    }
  }
  res = SPIFFS_rename(&s_fs, get_fixed_filename(from), get_fixed_filename(to));
  (void) r;
  set_errno(res);
  return res;
}

extern "C" int _unlink_r(struct _reent *r, const char *filename) {
  int res = SPIFFS_remove(&s_fs, get_fixed_filename(filename));
  (void) r;
  set_errno(res);

  return res;
}

extern "C" int _fstat_r(struct _reent *r, int fd, struct stat *s) {
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
  res = SPIFFS_fstat(&s_fs, fd - NUM_SYS_FD, &ss);
  set_errno(res);
  if (res < 0) return res;
  s->st_ino = ss.obj_id;
  s->st_mode = S_IFREG | 0666;
  s->st_nlink = 1;
  s->st_size = ss.size;
  return 0;
}

extern "C" int _stat_r(struct _reent *r, const char *path, struct stat *s) {
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
