#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "hw_types.h"
#include "hw_memmap.h"
#include "rom.h"
#include "rom_map.h"
#include "uart.h"

#include "config.h"
#include "cc3200_fs.h"
#include "cc3200_fs_spiffs.h"
#include "cc3200_fs_failfs.h"

#define NUM_SYS_FDS 3
#define SPIFFS_FD_BASE 10
#define FAILFS_FD_BASE 100

#if SPIFFS_FD_BASE + MAX_OPEN_SPIFFS_FILES >= FAILFS_FD_BASE
#error Too many MAX_OPEN_SPIFFS_FILES
#endif

int set_errno(int e) {
  errno = e;
  return -e;
}

static int is_ti_fname(const char *fname) {
  return strncmp(fname, "/ti/", 4) == 0;
}

static const char *ti_fname(const char *fname) {
  return fname + 4;
}

enum fd_type { FD_INVALID, FD_SYS, FD_SPIFFS, FD_FAILFS };
static int fd_type(int fd) {
  if (fd >= 0 && fd < NUM_SYS_FDS) return FD_SYS;
  if (fd >= SPIFFS_FD_BASE && fd < SPIFFS_FD_BASE + MAX_OPEN_SPIFFS_FILES) {
    return FD_SPIFFS;
  }
  if (fd >= FAILFS_FD_BASE && fd < FAILFS_FD_BASE + MAX_OPEN_FAILFS_FILES) {
    return FD_FAILFS;
  }
  return FD_INVALID;
}

int _open(const char *pathname, int flags, mode_t mode) {
  int fd;
  if (is_ti_fname(pathname)) {
    fd = fs_failfs_open(ti_fname(pathname), flags, mode);
    if (fd >= 0) fd += FAILFS_FD_BASE;
  } else {
    fd = fs_spiffs_open(pathname, flags, mode);
    if (fd >= 0) fd += SPIFFS_FD_BASE;
  }
  dprintf(("open(%s, 0x%x) = %d\n", pathname, flags, fd));
  return fd;
}

int _close(int fd) {
  int r = -1;
  switch (fd_type(fd)) {
    case FD_INVALID:
      r = set_errno(EBADF);
      break;
    case FD_SYS:
      r = set_errno(EACCES);
      break;
    case FD_SPIFFS:
      r = fs_spiffs_close(fd - SPIFFS_FD_BASE);
      break;
    case FD_FAILFS:
      r = fs_failfs_close(fd - FAILFS_FD_BASE);
      break;
  }
  dprintf(("close(%d) = %d\n", fd, r));
  return r;
}

off_t _lseek(int fd, off_t offset, int whence) {
  int r = -1;
  switch (fd_type(fd)) {
    case FD_INVALID:
      r = set_errno(EBADF);
      break;
    case FD_SYS:
      r = set_errno(ESPIPE);
      break;
    case FD_SPIFFS:
      r = fs_spiffs_lseek(fd - SPIFFS_FD_BASE, offset, whence);
      break;
    case FD_FAILFS:
      r = fs_failfs_lseek(fd - FAILFS_FD_BASE, offset, whence);
      break;
  }
  dprintf(("lseek(%d, %d, %d) = %d\n", fd, (int) offset, whence, r));
  return r;
}

int _fstat(int fd, struct stat *s) {
  int r = -1;
  switch (fd_type(fd)) {
    case FD_INVALID:
      r = set_errno(EBADF);
      break;
    case FD_SYS: {
      /* Create barely passable stats for STD{IN,OUT,ERR}. */
      memset(s, 0, sizeof(*s));
      s->st_ino = fd;
      s->st_rdev = fd;
      s->st_mode = S_IFCHR | 0666;
      r = 0;
      break;
    }
    case FD_SPIFFS:
      r = fs_spiffs_fstat(fd - SPIFFS_FD_BASE, s);
      break;
    case FD_FAILFS:
      r = fs_failfs_fstat(fd - FAILFS_FD_BASE, s);
      break;
  }
  dprintf(("fstat(%d) = %d\n", fd, r));
  return r;
}

ssize_t _read(int fd, void *buf, size_t count) {
  int r = -1;
  switch (fd_type(fd)) {
    case FD_INVALID:
      r = set_errno(EBADF);
      break;
    case FD_SYS: {
      if (fd != 0) {
        r = set_errno(EACCES);
        break;
      }
      /* Should we allow reading from stdin = uart? */
      r = set_errno(ENOTSUP);
      break;
    }
    case FD_SPIFFS:
      r = fs_spiffs_read(fd - SPIFFS_FD_BASE, buf, count);
      break;
    case FD_FAILFS:
      r = fs_failfs_read(fd - FAILFS_FD_BASE, buf, count);
      break;
  }
  dprintf(("read(%d, %u) = %d\n", fd, count, r));
  return r;
}

ssize_t _write(int fd, const void *buf, size_t count) {
  int r = -1;
  switch (fd_type(fd)) {
    case FD_INVALID:
      r = set_errno(EBADF);
      break;
    case FD_SYS: {
      if (fd == 0) {
        r = set_errno(EACCES);
        break;
      }
      for (size_t i = 0; i < count; i++) {
        const char c = ((const char *) buf)[i];
        if (c == '\n') MAP_UARTCharPut(CONSOLE_UART, '\r');
        MAP_UARTCharPut(CONSOLE_UART, c);
      }
      r = count;
      break;
    }
    case FD_SPIFFS:
      r = fs_spiffs_write(fd - SPIFFS_FD_BASE, buf, count);
      break;
    case FD_FAILFS:
      r = fs_failfs_write(fd - FAILFS_FD_BASE, buf, count);
      break;
  }
  return r;
}

int _rename(const char *from, const char *to) {
  int r;
  if (is_ti_fname(from) || is_ti_fname(to)) {
    r = set_errno(ENOTSUP);
  } else {
    r = fs_spiffs_rename(from, to);
  }
  dprintf(("rename(%s, %s) = %d\n", from, to, r));
  return r;
}

int _link(const char *from, const char *to) {
  dprintf(("link(%s, %s)\n", from, to));
  return set_errno(ENOTSUP);
}

int _unlink(const char *filename) {
  int r;
  if (is_ti_fname(filename)) {
    r = fs_failfs_unlink(ti_fname(filename));
  } else {
    r = fs_spiffs_unlink(filename);
  }
  dprintf(("unlink(%s) = %d\n", filename, r));
  return r;
}
