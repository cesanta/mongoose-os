/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/* LIBC interface to SPIFFS. */

#include <spiffs_vfs.h>

#if CS_SPIFFS_ENABLE_VFS

#include <errno.h>
#include <stdio.h>

#include <common/cs_dbg.h>

static int set_errno(int e) {
  errno = e;
  return (e == 0 ? 0 : -1);
}

static int spiffs_err_to_errno(int r) {
  switch (r) {
    case SPIFFS_OK:
      return 0;
    case SPIFFS_ERR_FULL:
      return ENOSPC;
    case SPIFFS_ERR_NOT_FOUND:
      return ENOENT;
    case SPIFFS_ERR_NOT_WRITABLE:
    case SPIFFS_ERR_NOT_READABLE:
      return EACCES;
  }
  return ENXIO;
}

int set_spiffs_errno(spiffs *fs, const char *op, int res) {
  int e = SPIFFS_errno(fs);
  LOG(LL_DEBUG, ("%s: res = %d, e = %d", op, res, e));
  if (res >= 0) return res;
  return set_errno(spiffs_err_to_errno(e));
}

int spiffs_vfs_open(spiffs *fs, const char *path, int flags, int mode) {
  spiffs_mode sm = 0;
  int rw = (flags & 3);
  if (rw == O_RDONLY || rw == O_RDWR) sm |= SPIFFS_RDONLY;
  if (rw == O_WRONLY || rw == O_RDWR) sm |= SPIFFS_WRONLY;
  if (flags & O_CREAT) sm |= SPIFFS_CREAT;
  if (flags & O_TRUNC) sm |= SPIFFS_TRUNC;
  if (flags & O_APPEND) sm |= SPIFFS_APPEND;
#ifdef O_EXCL
  if (flags & O_EXCL) sm |= SPIFFS_EXCL;
#endif

  return set_spiffs_errno(fs, path, SPIFFS_open(fs, path, sm, 0));
}

int spiffs_vfs_close(spiffs *fs, int fd) {
  return set_spiffs_errno(fs, "close", SPIFFS_close(fs, fd));
}

ssize_t spiffs_vfs_read(spiffs *fs, int fd, void *dst, size_t size) {
  int n = SPIFFS_read(fs, fd, dst, size);
  if (n < 0 && SPIFFS_errno(fs) == SPIFFS_ERR_END_OF_OBJECT) {
    /* EOF */
    n = 0;
  }
  return set_spiffs_errno(fs, "read", n);
}

size_t spiffs_vfs_write(spiffs *fs, int fd, const void *data, size_t size) {
  return set_spiffs_errno(fs, "write",
                          SPIFFS_write(fs, fd, (void *) data, size));
}

int spiffs_vfs_stat(spiffs *fs, const char *path, struct stat *st) {
  int res;
  spiffs_stat ss;
  memset(st, 0, sizeof(*st));
  res = SPIFFS_stat(fs, path, &ss);
  if (res == SPIFFS_OK) {
    st->st_ino = ss.obj_id;
    st->st_mode = S_IFREG | 0666;
    st->st_nlink = 1;
    st->st_size = ss.size;
  }
  return set_spiffs_errno(fs, "stat", res);
}

int spiffs_vfs_fstat(spiffs *fs, int fd, struct stat *st) {
  int res;
  spiffs_stat ss;
  memset(st, 0, sizeof(*st));
  res = SPIFFS_fstat(fs, fd, &ss);
  if (res == SPIFFS_OK) {
    st->st_ino = ss.obj_id;
    st->st_mode = S_IFREG | 0666;
    st->st_nlink = 1;
    st->st_size = ss.size;
  }
  return set_spiffs_errno(fs, "fstat", res);
}

off_t spiffs_vfs_lseek(spiffs *fs, int fd, off_t offset, int whence) {
  return set_spiffs_errno(fs, "lseek", SPIFFS_lseek(fs, fd, offset, whence));
}

int spiffs_vfs_rename(spiffs *fs, const char *src, const char *dst) {
  return set_spiffs_errno(fs, "rename", SPIFFS_rename(fs, src, dst));
}

int spiffs_vfs_unlink(spiffs *fs, const char *path) {
  return set_spiffs_errno(fs, "unlink", SPIFFS_remove(fs, path));
}

#endif /* CS_SPIFFS_ENABLE_VFS */
