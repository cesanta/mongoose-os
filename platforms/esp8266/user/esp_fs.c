/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#include <ets_sys.h>
#include <errno.h>
#include <fcntl.h>

#ifndef NO_V7
#include <v7.h>
#endif

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
#include "spiffs/spiffs_nucleus.h"
#include "spiffs_config.h"
#include "esp_uart.h"

#include "esp_fs.h"

#include <sys/mman.h>

/*
 * number of file descriptors reserved for system.
 * SPIFFS currently returns file descriptors that
 * clash with "system" fds like stdout and stderr.
 * Here we remap all spiffs fds by adding/subtracting NUM_SYS_FD
 */
#define NUM_SYS_FD 3

spiffs fs;

#define FLASH_BLOCK_SIZE (4 * 1024)
#define FLASH_UNIT_SIZE 4

#ifndef FS_MAX_OPEN_FILES
#define FS_MAX_OPEN_FILES 10
#endif

#define DUMMY_MMAP_BUFFER_START ((u8_t *) 0x70000000)
#define DUMMY_MMAP_BUFFER_END ((u8_t *) 0x70100000)

struct mmap_desc mmap_descs[SJ_MMAP_SLOTS];
static struct mmap_desc *cur_mmap_desc;

static u8_t spiffs_work_buf[LOG_PAGE_SIZE * 2];
static u8_t spiffs_fds[32 * FS_MAX_OPEN_FILES];

int spiffs_get_memory_usage() {
  return sizeof(spiffs_work_buf) + sizeof(spiffs_fds);
}

static struct mmap_desc *alloc_mmap_desc() {
  size_t i;
  for (i = 0; i < sizeof(mmap_descs) / sizeof(mmap_descs[0]); i++) {
    if (mmap_descs[i].blocks == NULL) {
      return &mmap_descs[i];
    }
  }
  return NULL;
}

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset) {
  int pages = (len + LOG_PAGE_SIZE - 1) / LOG_PAGE_SIZE;
  struct mmap_desc *desc = alloc_mmap_desc();
  if (desc == NULL) {
    fprintf(stderr, "cannot allocate mmap desc\n");
    return MAP_FAILED;
  }

  cur_mmap_desc = desc;
  desc->pages = 0;
  desc->blocks = (uint32_t *) calloc(sizeof(uint32_t), pages);
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
  int i, j;
  for (i = 0; i < (int) (sizeof(mmap_descs) / sizeof(mmap_descs[0])); i++) {
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
#ifdef CS_MMAP
  if (dst >= DUMMY_MMAP_BUFFER_START && dst < DUMMY_MMAP_BUFFER_END) {
    if ((addr - SPIFFS_PAGE_HEADER_SIZE) % LOG_PAGE_SIZE == 0) {
      fprintf(stderr, "mmap spiffs prep read: %x %u %p\n", addr, size, dst);
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

int fs_mount(spiffs *spf, uint32_t addr, uint32_t size, uint8_t *workbuf,
             uint8_t *fds, size_t fds_size) {
  spiffs_config cfg;

  /* FS_SIZE & FS_ADDR are provided via Makefile */
  cfg.phys_addr = addr;
  cfg.phys_size = size;

  cfg.phys_erase_block = FLASH_BLOCK_SIZE;
  cfg.log_block_size = FLASH_BLOCK_SIZE;
  cfg.log_page_size = LOG_PAGE_SIZE;

  cfg.hal_read_f = esp_spiffs_read;
  cfg.hal_write_f = esp_spiffs_write;
  cfg.hal_erase_f = esp_spiffs_erase;

  if (SPIFFS_mount(spf, &cfg, workbuf, fds, fds_size, 0, 0, 0) != SPIFFS_OK) {
    return SPIFFS_errno(spf);
  }

  return 0;
}

int fs_init(uint32_t addr, uint32_t size) {
  return fs_mount(&fs, addr, size, spiffs_work_buf, spiffs_fds,
                  sizeof(spiffs_fds));
}

/* Wrappers for V7 */

void set_errno(int res) {
  if (res < 0) {
    errno = SPIFFS_errno(&fs);
    fprintf(stderr, "spiffs error: %d\n", errno);
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
  if (rw == O_WRONLY || rw == O_RDWR) sm |= SPIFFS_WRONLY | SPIFFS_CREAT;
  if (flags & O_CREAT) sm |= SPIFFS_CREAT;
  if (flags & O_TRUNC) sm |= SPIFFS_TRUNC;
  if (flags & O_APPEND) sm |= SPIFFS_APPEND | SPIFFS_CREAT;

  /* Supported in newer versions of SPIFFS. */
  /* if (flags && O_EXCL) sm |= SPIFFS_EXCL; */
  /* if (flags && O_DIRECT) sm |= SPIFFS_DIRECT; */

  /* spiffs doesn't support directories, not even the trivial ./something */
  while (*filename != '\0' && (*filename == '/' || *filename == '.')) {
    filename++;
  }

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

int _stat_r(struct _reent *r, const char *path, struct stat *s) {
  int ret, fd;

  /*
   * spiffs has no directories, simulating statting root directory;
   * required for mg_send_http_file.
   */
  if ((strcmp(path, "./") == 0) || (strcmp(path, "/") == 0)) {
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

#ifndef NO_V7
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

#endif
