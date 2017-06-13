/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_vfs.h"

/* libc interfaces are defined slightly differently for Newlib (when compiling
 * with GCC) and TI's libc (when compiling with TICC). */

#ifdef __TI_COMPILER_VERSION__
int open(const char *path, unsigned int flags, int mode) {
#else
int _open(const char *path, int flags, mode_t mode) {
#endif
  return mgos_vfs_open(path, (int) flags, mode);
}

#ifdef __TI_COMPILER_VERSION__
int close(int vfd) {
#else
int _close(int vfd) {
#endif
  return mgos_vfs_close(vfd);
}

#ifdef __TI_COMPILER_VERSION__
int read(int vfd, char *dst, unsigned int len) {
#else
ssize_t _read(int vfd, void *dst, size_t len) {
#endif
  return mgos_vfs_read(vfd, dst, len);
}

#ifdef __TI_COMPILER_VERSION__
int write(int vfd, const char *src, unsigned int len) {
#else
ssize_t _write(int vfd, const void *src, size_t len) {
#endif
  return mgos_vfs_write(vfd, src, len);
}

#ifdef __TI_COMPILER_VERSION__
int stat(const char *path, struct stat *st) {
#else
int _stat(const char *path, struct stat *st) {
#endif
  return mgos_vfs_stat(path, st);
}

#ifdef __TI_COMPILER_VERSION__
int fstat(int vfd, struct stat *st) {
#else
int _fstat(int vfd, struct stat *st) {
#endif
  return mgos_vfs_fstat(vfd, st);
}

#ifdef __TI_COMPILER_VERSION__
off_t lseek(int vfd, off_t offset, int whence) {
#else
off_t _lseek(int vfd, off_t offset, int whence) {
#endif
  return mgos_vfs_lseek(vfd, offset, whence);
}

#ifdef __TI_COMPILER_VERSION__
int unlink(const char *path) {
#else
int _unlink(const char *path) {
#endif
  return mgos_vfs_unlink(path);
}

/*
 * On Newlib we override rename directly too, because the default
 * implementation using _link and _unlink doesn't work for us.
 */
int rename(const char *src, const char *dst) {
  return mgos_vfs_rename(src, dst);
}
