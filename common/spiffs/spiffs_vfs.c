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

static const char *drop_dir(const char *fname) {
  const char *old_fname = fname;
  /* Drop "./", if any */
  if (fname[0] == '.' && fname[1] == '/') {
    fname += 2;
  }
  /*
   * Drop / if it is the only one in the path.
   * This allows use of /pretend/directories but serves /file.txt as normal.
   */
  if (fname[0] == '/' && strchr(fname + 1, '/') == NULL) {
    fname++;
  }
  if (fname != old_fname) {
    LOG(LL_DEBUG, ("'%s' -> '%s'", old_fname, fname));
  }
  return fname;
}

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

  (void) mode;
  return set_spiffs_errno(fs, path, SPIFFS_open(fs, drop_dir(path), sm, 0));
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
  const char *fname = drop_dir(path);
  /* Simulate statting the root directory. */
  if (fname[0] == '\0' || strcmp(fname, ".") == 0) {
    st->st_ino = 0;
    st->st_mode = S_IFDIR | 0777;
    st->st_nlink = 1;
    st->st_size = 0;
    return set_spiffs_errno(fs, path, SPIFFS_OK);
  }
  res = SPIFFS_stat(fs, fname, &ss);
  if (res == SPIFFS_OK) {
    st->st_ino = ss.obj_id;
    st->st_mode = S_IFREG | 0666;
    st->st_nlink = 1;
    st->st_size = ss.size;
  }
  return set_spiffs_errno(fs, path, res);
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
  int res;
  /* Renaming file to itself should be a no-op. */
  src = drop_dir(src);
  dst = drop_dir(dst);
  if (strcmp(src, dst) == 0) return 0;
  {
    /*
     * POSIX rename requires that in case "to" exists, it be atomically replaced
     * with "from". The atomic part we can't do, but at least we can do replace.
     */
    spiffs_stat ss;
    res = SPIFFS_stat(fs, dst, &ss);
    if (res == 0) {
      SPIFFS_remove(fs, dst);
    }
  }
  return set_spiffs_errno(fs, "rename", SPIFFS_rename(fs, src, dst));
}

int spiffs_vfs_unlink(spiffs *fs, const char *path) {
  return set_spiffs_errno(fs, "unlink", SPIFFS_remove(fs, drop_dir(path)));
}

#endif /* CS_SPIFFS_ENABLE_VFS */
