/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/cc3200/src/cc3200_vfs_fs_slfs.h"

#include <stdio.h>
#include <stdlib.h>

#include "common/platform.h"
#include "common/platforms/simplelink/sl_fs_slfs.h"

#include "fw/src/mgos_vfs.h"

bool s_slfs_mounted = false;

static bool cc3200_vfs_fs_slfs_mkfs(struct mgos_vfs_fs *fs, const char *opts) {
  /* Not supported */
  (void) fs;
  (void) opts;
  return false;
}

static bool cc3200_vfs_fs_slfs_mount(struct mgos_vfs_fs *fs, const char *opts) {
  (void) fs;
  (void) opts;
  if (s_slfs_mounted) return false;
  s_slfs_mounted = true;
  return true;
}

bool cc3200_vfs_fs_slfs_umount(struct mgos_vfs_fs *fs) {
  /* Nothing to do. */
  (void) fs;
  if (!s_slfs_mounted) return false;
  s_slfs_mounted = false;
  return true;
}

size_t cc3200_vfs_fs_slfs_get_space_total(struct mgos_vfs_fs *fs) {
  /* Not supported. */
  (void) fs;
  return 0;
}

size_t cc3200_vfs_fs_slfs_get_space_used(struct mgos_vfs_fs *fs) {
  /* Not supported. */
  (void) fs;
  return 0;
}

size_t cc3200_vfs_fs_slfs_get_space_free(struct mgos_vfs_fs *fs) {
  /* Not supported. */
  (void) fs;
  return 0;
}

bool cc3200_vfs_fs_slfs_gc(struct mgos_vfs_fs *fs) {
  /* Nothing to do. */
  (void) fs;
  return true;
}

int cc3200_vfs_fs_slfs_open(struct mgos_vfs_fs *fs, const char *path, int flags,
                            int mode) {
  (void) fs;
  return fs_slfs_open(path, flags, mode);
}

int cc3200_vfs_fs_slfs_close(struct mgos_vfs_fs *fs, int fd) {
  (void) fs;
  return fs_slfs_close(fd);
}

ssize_t cc3200_vfs_fs_slfs_read(struct mgos_vfs_fs *fs, int fd, void *dst,
                                size_t size) {
  (void) fs;
  return fs_slfs_read(fd, dst, size);
}

ssize_t cc3200_vfs_fs_slfs_write(struct mgos_vfs_fs *fs, int fd,
                                 const void *src, size_t size) {
  (void) fs;
  return fs_slfs_write(fd, src, size);
}

int cc3200_vfs_fs_slfs_stat(struct mgos_vfs_fs *fs, const char *path,
                            struct stat *st) {
  (void) fs;
  return fs_slfs_stat(path, st);
}

int cc3200_vfs_fs_slfs_fstat(struct mgos_vfs_fs *fs, int fd, struct stat *st) {
  (void) fs;
  return fs_slfs_fstat(fd, st);
}

off_t cc3200_vfs_fs_slfs_lseek(struct mgos_vfs_fs *fs, int fd, off_t offset,
                               int whence) {
  (void) fs;
  return fs_slfs_lseek(fd, offset, whence);
}

int cc3200_vfs_fs_slfs_rename(struct mgos_vfs_fs *fs, const char *src,
                              const char *dst) {
  (void) fs;
  return fs_slfs_rename(src, dst);
}

int cc3200_vfs_fs_slfs_unlink(struct mgos_vfs_fs *fs, const char *path) {
  (void) fs;
  return fs_slfs_unlink(path);
}

#if MG_ENABLE_DIRECTORY_LISTING
/* SLFS does not support listing files. */
static DIR *cc3200_vfs_fs_slfs_opendir(struct mgos_vfs_fs *fs,
                                       const char *path) {
  errno = ENOTSUP;
  return NULL;
}

static struct dirent *cc3200_vfs_fs_slfs_readdir(struct mgos_vfs_fs *fs,
                                                 DIR *dir) {
  errno = ENOTSUP;
  return NULL;
}

static int cc3200_vfs_fs_slfs_closedir(struct mgos_vfs_fs *fs, DIR *dir) {
  errno = ENOTSUP;
  return -1;
}
#endif /* MG_ENABLE_DIRECTORY_LISTING */

static const struct mgos_vfs_fs_ops cc3200_vfs_fs_slfs_ops = {
    .mkfs = cc3200_vfs_fs_slfs_mkfs,
    .mount = cc3200_vfs_fs_slfs_mount,
    .umount = cc3200_vfs_fs_slfs_umount,
    .get_space_total = cc3200_vfs_fs_slfs_get_space_total,
    .get_space_used = cc3200_vfs_fs_slfs_get_space_used,
    .get_space_free = cc3200_vfs_fs_slfs_get_space_free,
    .gc = cc3200_vfs_fs_slfs_gc,
    .open = cc3200_vfs_fs_slfs_open,
    .close = cc3200_vfs_fs_slfs_close,
    .read = cc3200_vfs_fs_slfs_read,
    .write = cc3200_vfs_fs_slfs_write,
    .stat = cc3200_vfs_fs_slfs_stat,
    .fstat = cc3200_vfs_fs_slfs_fstat,
    .lseek = cc3200_vfs_fs_slfs_lseek,
    .unlink = cc3200_vfs_fs_slfs_unlink,
    .rename = cc3200_vfs_fs_slfs_rename,
#if MG_ENABLE_DIRECTORY_LISTING
    .opendir = cc3200_vfs_fs_slfs_opendir,
    .readdir = cc3200_vfs_fs_slfs_readdir,
    .closedir = cc3200_vfs_fs_slfs_closedir,
#endif
};

bool cc3200_vfs_fs_slfs_register_type(void) {
  return mgos_vfs_fs_register_type(MGOS_VFS_FS_TYPE_SLFS,
                                   &cc3200_vfs_fs_slfs_ops);
}
