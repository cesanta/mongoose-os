/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if defined(MG_SOCKET_SIMPLELINK) && \
    (defined(MG_FS_SLFS) || defined(MG_FS_SPIFFS))

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __TI_COMPILER_VERSION__
#include <file.h>
#endif

#include "common/cs_dbg.h"
#include "common/platform.h"

#ifdef CC3200_FS_SPIFFS
#include "cc3200_fs_spiffs.h"
#endif

#ifdef MG_FS_SLFS
#include "sl_fs_slfs.h"
#endif

#define NUM_SYS_FDS 3
#define SPIFFS_FD_BASE 10
#define SLFS_FD_BASE 100

#ifndef MG_UART_CHAR_PUT
#if CS_PLATFORM == CS_P_CC3200
#include <inc/hw_types.h>
#include <inc/hw_memmap.h>
#include <driverlib/rom.h>
#include <driverlib/rom_map.h>
#include <driverlib/uart.h>
#define MG_UART_CHAR_PUT(fd, c) MAP_UARTCharPut(UARTA0_BASE, c);
#else
#define MG_UART_CHAR_PUT(fd, c)
#endif /* CS_PLATFORM == CS_P_CC3200 */
#endif /* !MG_UART_CHAR_PUT */

int set_errno(int e) {
  errno = e;
  return (e == 0 ? 0 : -1);
}

static int is_sl_fname(const char *fname) {
  return strncmp(fname, "SL:", 3) == 0;
}

static const char *sl_fname(const char *fname) {
  return fname + 3;
}

static const char *drop_dir(const char *fname) {
  if (*fname == '.') fname++;
  if (*fname == '/') fname++;
  return fname;
}

enum fd_type {
  FD_INVALID,
  FD_SYS,
#ifdef CC3200_FS_SPIFFS
  FD_SPIFFS,
#endif
#ifdef MG_FS_SLFS
  FD_SLFS
#endif
};
static int fd_type(int fd) {
  if (fd >= 0 && fd < NUM_SYS_FDS) return FD_SYS;
#ifdef CC3200_FS_SPIFFS
  if (fd >= SPIFFS_FD_BASE && fd < SPIFFS_FD_BASE + MAX_OPEN_SPIFFS_FILES) {
    return FD_SPIFFS;
  }
#endif
#ifdef MG_FS_SLFS
  if (fd >= SLFS_FD_BASE && fd < SLFS_FD_BASE + MAX_OPEN_SLFS_FILES) {
    return FD_SLFS;
  }
#endif
  return FD_INVALID;
}

#if MG_TI_NO_HOST_INTERFACE
int open(const char *pathname, unsigned flags, int mode) {
#else
int _open(const char *pathname, int flags, mode_t mode) {
#endif
  int fd = -1;
  pathname = drop_dir(pathname);
  if (is_sl_fname(pathname)) {
#ifdef MG_FS_SLFS
    fd = fs_slfs_open(sl_fname(pathname), flags, mode);
    if (fd >= 0) fd += SLFS_FD_BASE;
#endif
  } else {
#ifdef CC3200_FS_SPIFFS
    fd = fs_spiffs_open(pathname, flags, mode);
    if (fd >= 0) fd += SPIFFS_FD_BASE;
#endif
  }
  DBG(("open(%s, 0x%x) = %d", pathname, flags, fd));
  return fd;
}

int _stat(const char *pathname, struct stat *st) {
  int res = -1;
  const char *fname = pathname;
  int is_sl = is_sl_fname(pathname);
  if (is_sl) fname = sl_fname(pathname);
  fname = drop_dir(fname);
  memset(st, 0, sizeof(*st));
  /* Simulate statting the root directory. */
  if (strcmp(fname, "") == 0) {
    st->st_ino = 0;
    st->st_mode = S_IFDIR | 0777;
    st->st_nlink = 1;
    st->st_size = 0;
    return 0;
  }
  if (is_sl) {
#ifdef MG_FS_SLFS
    res = fs_slfs_stat(fname, st);
#endif
  } else {
#ifdef CC3200_FS_SPIFFS
    res = fs_spiffs_stat(fname, st);
#endif
  }
  DBG(("stat(%s) = %d; fname = %s", pathname, res, fname));
  return res;
}

#if MG_TI_NO_HOST_INTERFACE
int close(int fd) {
#else
int _close(int fd) {
#endif
  int r = -1;
  switch (fd_type(fd)) {
    case FD_INVALID:
      r = set_errno(EBADF);
      break;
    case FD_SYS:
      r = set_errno(EACCES);
      break;
#ifdef CC3200_FS_SPIFFS
    case FD_SPIFFS:
      r = fs_spiffs_close(fd - SPIFFS_FD_BASE);
      break;
#endif
#ifdef MG_FS_SLFS
    case FD_SLFS:
      r = fs_slfs_close(fd - SLFS_FD_BASE);
      break;
#endif
  }
  DBG(("close(%d) = %d", fd, r));
  return r;
}

#if MG_TI_NO_HOST_INTERFACE
off_t lseek(int fd, off_t offset, int whence) {
#else
off_t _lseek(int fd, off_t offset, int whence) {
#endif
  int r = -1;
  switch (fd_type(fd)) {
    case FD_INVALID:
      r = set_errno(EBADF);
      break;
    case FD_SYS:
      r = set_errno(ESPIPE);
      break;
#ifdef CC3200_FS_SPIFFS
    case FD_SPIFFS:
      r = fs_spiffs_lseek(fd - SPIFFS_FD_BASE, offset, whence);
      break;
#endif
#ifdef MG_FS_SLFS
    case FD_SLFS:
      r = fs_slfs_lseek(fd - SLFS_FD_BASE, offset, whence);
      break;
#endif
  }
  DBG(("lseek(%d, %d, %d) = %d", fd, (int) offset, whence, r));
  return r;
}

int _fstat(int fd, struct stat *s) {
  int r = -1;
  memset(s, 0, sizeof(*s));
  switch (fd_type(fd)) {
    case FD_INVALID:
      r = set_errno(EBADF);
      break;
    case FD_SYS: {
      /* Create barely passable stats for STD{IN,OUT,ERR}. */
      memset(s, 0, sizeof(*s));
      s->st_ino = fd;
      s->st_mode = S_IFCHR | 0666;
      r = 0;
      break;
    }
#ifdef CC3200_FS_SPIFFS
    case FD_SPIFFS:
      r = fs_spiffs_fstat(fd - SPIFFS_FD_BASE, s);
      break;
#endif
#ifdef MG_FS_SLFS
    case FD_SLFS:
      r = fs_slfs_fstat(fd - SLFS_FD_BASE, s);
      break;
#endif
  }
  DBG(("fstat(%d) = %d", fd, r));
  return r;
}

#if MG_TI_NO_HOST_INTERFACE
int read(int fd, char *buf, unsigned count) {
#else
ssize_t _read(int fd, void *buf, size_t count) {
#endif
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
#ifdef CC3200_FS_SPIFFS
    case FD_SPIFFS:
      r = fs_spiffs_read(fd - SPIFFS_FD_BASE, buf, count);
      break;
#endif
#ifdef MG_FS_SLFS
    case FD_SLFS:
      r = fs_slfs_read(fd - SLFS_FD_BASE, buf, count);
      break;
#endif
  }
  DBG(("read(%d, %u) = %d", fd, count, r));
  return r;
}

#if MG_TI_NO_HOST_INTERFACE
int write(int fd, const char *buf, unsigned count) {
#else
ssize_t _write(int fd, const void *buf, size_t count) {
#endif
  int r = -1;
  size_t i = 0;
  switch (fd_type(fd)) {
    case FD_INVALID:
      r = set_errno(EBADF);
      break;
    case FD_SYS: {
      if (fd == 0) {
        r = set_errno(EACCES);
        break;
      }
      for (i = 0; i < count; i++) {
        const char c = ((const char *) buf)[i];
        if (c == '\n') MG_UART_CHAR_PUT(fd, '\r');
        MG_UART_CHAR_PUT(fd, c);
      }
      r = count;
      break;
    }
#ifdef CC3200_FS_SPIFFS
    case FD_SPIFFS:
      r = fs_spiffs_write(fd - SPIFFS_FD_BASE, buf, count);
      break;
#endif
#ifdef MG_FS_SLFS
    case FD_SLFS:
      r = fs_slfs_write(fd - SLFS_FD_BASE, buf, count);
      break;
#endif
  }
  return r;
}

/*
 * On Newlib we override rename directly too, because the default
 * implementation using _link and _unlink doesn't work for us.
 */
#if MG_TI_NO_HOST_INTERFACE || defined(_NEWLIB_VERSION)
int rename(const char *from, const char *to) {
  int r = -1;
  from = drop_dir(from);
  to = drop_dir(to);
  if (is_sl_fname(from) || is_sl_fname(to)) {
#ifdef MG_FS_SLFS
    r = fs_slfs_rename(sl_fname(from), sl_fname(to));
#endif
  } else {
#ifdef CC3200_FS_SPIFFS
    r = fs_spiffs_rename(from, to);
#endif
  }
  DBG(("rename(%s, %s) = %d", from, to, r));
  return r;
}
#endif /* MG_TI_NO_HOST_INTERFACE || defined(_NEWLIB_VERSION) */

#if MG_TI_NO_HOST_INTERFACE
int unlink(const char *filename) {
#else
int _unlink(const char *filename) {
#endif
  int r = -1;
  filename = drop_dir(filename);
  if (is_sl_fname(filename)) {
#ifdef MG_FS_SLFS
    r = fs_slfs_unlink(sl_fname(filename));
#endif
  } else {
#ifdef CC3200_FS_SPIFFS
    r = fs_spiffs_unlink(filename);
#endif
  }
  DBG(("unlink(%s) = %d", filename, r));
  return r;
}

#ifdef CC3200_FS_SPIFFS /* FailFS does not support listing files. */
DIR *opendir(const char *dir_name) {
  DIR *r = NULL;
  if (is_sl_fname(dir_name)) {
    r = NULL;
    set_errno(ENOTSUP);
  } else {
    r = fs_spiffs_opendir(dir_name);
  }
  DBG(("opendir(%s) = %p", dir_name, r));
  return r;
}

struct dirent *readdir(DIR *dir) {
  struct dirent *res = fs_spiffs_readdir(dir);
  DBG(("readdir(%p) = %p", dir, res));
  return res;
}

int closedir(DIR *dir) {
  int res = fs_spiffs_closedir(dir);
  DBG(("closedir(%p) = %d", dir, res));
  return res;
}

int rmdir(const char *path) {
  return fs_spiffs_rmdir(path);
}

int mkdir(const char *path, mode_t mode) {
  (void) path;
  (void) mode;
  /* for spiffs supports only root dir, which comes from mongoose as '.' */
  return (strlen(path) == 1 && *path == '.') ? 0 : ENOTDIR;
}
#endif

int sl_fs_init(void) {
  int ret = 1;
#ifdef __TI_COMPILER_VERSION__
#ifdef MG_FS_SLFS
#pragma diag_push
#pragma diag_suppress 169 /* Nothing we can do about the prototype mismatch. */
  ret = (add_device("SL", _MSA, fs_slfs_open, fs_slfs_close, fs_slfs_read,
                    fs_slfs_write, fs_slfs_lseek, fs_slfs_unlink,
                    fs_slfs_rename) == 0);
#pragma diag_pop
#endif
#endif
  return ret;
}

#endif /* defined(MG_SOCKET_SIMPLELINK) && (defined(MG_FS_SLFS) || \
          defined(MG_FS_SPIFFS)) */
