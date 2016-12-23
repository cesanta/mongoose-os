/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#include <ets_sys.h>
#include <errno.h>
#include <fcntl.h>

#include "fw/src/miot_features.h"

#if MIOT_ENABLE_JS
#include "v7/v7.h"
#endif

#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "mem.h"
#include <errno.h>
#include <fcntl.h>

#include "common/cs_dbg.h"
#include "common/cs_dirent.h"

#include "common/spiffs/spiffs.h"
#include "common/spiffs/spiffs_nucleus.h"
#include "spiffs_config.h"

#include "esp_fs.h"
#include "fw/src/miot_uart.h"
#include "fw/src/miot_uart.h"
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

#define DUMMY_MMAP_BUFFER_START ((u8_t *) 0x70000000)
#define DUMMY_MMAP_BUFFER_END ((u8_t *) 0x70100000)

struct mmap_desc mmap_descs[MIOT_MMAP_SLOTS];
static struct mmap_desc *cur_mmap_desc;

static u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
static u8_t spiffs_fds[32 * FS_MAX_OPEN_FILES];

static int8_t s_stdout_uart = MIOT_DEBUG_UART;
static int8_t s_stderr_uart = MIOT_DEBUG_UART;

/* For cs_dirent.c functions */
spiffs *cs_spiffs_get_fs(void) {
  return &fs;
}

int spiffs_get_memory_usage(void) {
  return sizeof(spiffs_work_buf) + sizeof(spiffs_fds);
}

static struct mmap_desc *alloc_mmap_desc(void) {
  size_t i;
  for (i = 0; i < sizeof(mmap_descs) / sizeof(mmap_descs[0]); i++) {
    if (mmap_descs[i].blocks == NULL) {
      return &mmap_descs[i];
    }
  }
  return NULL;
}

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
  int pages = (len + SPIFFS_PAGE_DATA_SIZE - 1) / SPIFFS_PAGE_DATA_SIZE;
  struct mmap_desc *desc = alloc_mmap_desc();
  (void) addr;
  (void) prot;
  (void) flags;
  (void) offset;

  if (len == 0) {
    return NULL;
  }

  if (desc == NULL) {
    LOG(LL_ERROR, ("cannot allocate mmap desc"));
    return MAP_FAILED;
  }

  cur_mmap_desc = desc;
  desc->pages = 0;
  desc->blocks = (uint32_t *) calloc(sizeof(uint32_t), pages);
  if (desc->blocks == NULL) {
    LOG(LL_ERROR, ("Out of memory"));
    return MAP_FAILED;
  }
  desc->base = MMAP_ADDR_FROM_DESC(desc);
  SPIFFS_read(&fs, fd - NUM_SYS_FD, DUMMY_MMAP_BUFFER_START, len);
  /*
   * this breaks the posix-like mmap abstraction but file descriptors are a
   * scarse resource here.
   */
  SPIFFS_close(&fs, fd - NUM_SYS_FD);

  return desc->base;
}

/*
 * Relocate mmapped pages.
 */
void esp_spiffs_on_page_move_hook(spiffs *fs, spiffs_file fh,
                                  spiffs_page_ix src_pix,
                                  spiffs_page_ix dst_pix) {
  size_t i, j;
  (void) fh;
  /* for (i = 0; i < (sizeof(mmap_descs) / sizeof(mmap_descs[0])); i++) { */
  for (i = 0; i < ARRAY_SIZE(mmap_descs); i++) {
    if (mmap_descs[i].blocks) {
      for (j = 0; j < mmap_descs[i].pages; j++) {
        uint32_t addr = mmap_descs[i].blocks[j];
        uint32_t page = SPIFFS_PADDR_TO_PAGE(fs, addr - FLASH_BASE);
        if (page == src_pix) {
          int delta = (int) dst_pix - (int) src_pix;
          mmap_descs[i].blocks[j] += delta * LOG_PAGE_SIZE;
        }
      }
    }
  }
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
#ifdef CS_MMAP
  if (dst >= DUMMY_MMAP_BUFFER_START && dst < DUMMY_MMAP_BUFFER_END) {
    if ((addr - SPIFFS_PAGE_HEADER_SIZE) % LOG_PAGE_SIZE == 0) {
      /*
       * If FW uses OTA (and flash mapping) addr might be > 0x100000
       * and FLASH_BASE + addr will point somewhere behind flash
       * mapped area (40200000h-40300000h)
       * So, we need map it back.
       * (i.e. if addr > 0x100000 -> addr -= 0x100000)
       */
      addr &= 0xFFFFF;
      cur_mmap_desc->blocks[cur_mmap_desc->pages++] = FLASH_BASE + addr;
    }
    return SPIFFS_OK;
  }
#endif

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
  (void) fs;
}

static s32_t esp_spiffs_write(spiffs *fs, u32_t addr, u32_t size, u8_t *src) {
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

/* Wrappers for V7 */

void set_errno(int res) {
  if (res < 0) {
    errno = SPIFFS_errno(&fs);
    /* NOTE(lsm): use DEBUG level, as "not found" error -10002 is too noisy */
    LOG(LL_DEBUG, ("spiffs error: %d", errno));
  }
}

void add_plus(char *ptr, int *open_mode) {
  if (*(ptr + 1) == '+') {
    *open_mode |= SPIFFS_RDWR;
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
    if (uart_no >= 0) len = miot_uart_write(uart_no, buf, len);
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
  (void) r;
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

void fs_flush_stderr(void) {
  if (s_stderr_uart >= 0) miot_uart_flush(s_stderr_uart);
}

#if MIOT_ENABLE_JS
int v7_val_to_file(struct v7 *v7, v7_val_t val) {
  return (int) v7_get_double(v7, val);
}

v7_val_t v7_file_to_val(struct v7 *v7, int fd) {
  (void) v7;
  return v7_mk_number(v7, fd);
}

int v7_is_file_type(v7_val_t val) {
  int res = v7_is_number(val);
  return res;
}
#endif

int64_t miot_get_storage_free_space(void) {
  uint32_t total, used;
  SPIFFS_info(&fs, &total, &used);
  return total - used;
}

enum miot_init_result miot_set_stdout_uart(int uart_no) {
  enum miot_init_result r = miot_init_debug_uart(uart_no);
  if (r == MIOT_INIT_OK) {
    s_stdout_uart = uart_no;
  }
  return r;
}

enum miot_init_result miot_set_stderr_uart(int uart_no) {
  enum miot_init_result r = miot_init_debug_uart(uart_no);
  if (r == MIOT_INIT_OK) {
    s_stderr_uart = uart_no;
  }
  return r;
}

enum miot_init_result esp_console_init() {
  return miot_init_debug_uart(MIOT_DEBUG_UART);
}
