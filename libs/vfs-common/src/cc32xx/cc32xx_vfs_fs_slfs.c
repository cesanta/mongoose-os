/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cc32xx_vfs_fs_slfs.h"

#include <stdio.h>
#include <stdlib.h>

#include "common/str_util.h"
#include "common/platform.h"
#include "common/platforms/simplelink/sl_fs_slfs.h"

#include "mgos_vfs.h"

bool s_slfs_mounted = false;

static bool cc32xx_vfs_fs_slfs_mkfs(struct mgos_vfs_fs *fs, const char *opts) {
  /* Not supported */
  (void) fs;
  (void) opts;
  return false;
}

static bool cc32xx_vfs_fs_slfs_mount(struct mgos_vfs_fs *fs, const char *opts) {
  (void) fs;
  (void) opts;
  if (s_slfs_mounted) return false;
  s_slfs_mounted = true;
  return true;
}

bool cc32xx_vfs_fs_slfs_umount(struct mgos_vfs_fs *fs) {
  /* Nothing to do. */
  (void) fs;
  if (!s_slfs_mounted) return false;
  s_slfs_mounted = false;
  return true;
}

static void slfs_get_fs_info(size_t *bytes_total, size_t *bytes_used,
                             size_t *bytes_free) {
  *bytes_total = *bytes_used = *bytes_free = 0;
#if SL_MAJOR_VERSION_NUM >= 2
  SlFsControlGetStorageInfoResponse_t si;
  if (sl_FsCtl((SlFsCtl_e) SL_FS_CTL_GET_STORAGE_INFO, 0, NULL, NULL, 0,
               (_u8 *) &si, sizeof(si), NULL) == 0) {
    _u32 bs = si.DeviceUsage.DeviceBlockSize;
    *bytes_total = si.DeviceUsage.DeviceBlocksCapacity *bs;
    *bytes_free = si.DeviceUsage.NumOfAvailableBlocksForUserFiles *bs;
    *bytes_used = *bytes_total - *bytes_free;
  }
#endif
}

size_t cc32xx_vfs_fs_slfs_get_space_total(struct mgos_vfs_fs *fs) {
  size_t bytes_total, bytes_used, bytes_free;
  slfs_get_fs_info(&bytes_total, &bytes_used, &bytes_free);
  (void) fs;
  return bytes_total;
}

size_t cc32xx_vfs_fs_slfs_get_space_used(struct mgos_vfs_fs *fs) {
  size_t bytes_total, bytes_used, bytes_free;
  slfs_get_fs_info(&bytes_total, &bytes_used, &bytes_free);
  (void) fs;
  return bytes_used;
}

size_t cc32xx_vfs_fs_slfs_get_space_free(struct mgos_vfs_fs *fs) {
  size_t bytes_total, bytes_used, bytes_free;
  slfs_get_fs_info(&bytes_total, &bytes_used, &bytes_free);
  (void) fs;
  return bytes_free;
}

bool cc32xx_vfs_fs_slfs_gc(struct mgos_vfs_fs *fs) {
  /* Nothing to do. */
  (void) fs;
  return true;
}

int cc32xx_vfs_fs_slfs_open(struct mgos_vfs_fs *fs, const char *path, int flags,
                            int mode) {
  char buf[32], *slash_path = buf;
  mg_asprintf(&slash_path, sizeof(buf), "/%s", path);
  int ret = fs_slfs_open(slash_path, flags, mode);
  if (slash_path != buf) free(slash_path);
  (void) fs;
  return ret;
}

int cc32xx_vfs_fs_slfs_close(struct mgos_vfs_fs *fs, int fd) {
  (void) fs;
  return fs_slfs_close(fd);
}

ssize_t cc32xx_vfs_fs_slfs_read(struct mgos_vfs_fs *fs, int fd, void *dst,
                                size_t size) {
  (void) fs;
  return fs_slfs_read(fd, dst, size);
}

ssize_t cc32xx_vfs_fs_slfs_write(struct mgos_vfs_fs *fs, int fd,
                                 const void *src, size_t size) {
  (void) fs;
  return fs_slfs_write(fd, src, size);
}

int cc32xx_vfs_fs_slfs_stat(struct mgos_vfs_fs *fs, const char *path,
                            struct stat *st) {
  (void) fs;
  return fs_slfs_stat(path, st);
}

int cc32xx_vfs_fs_slfs_fstat(struct mgos_vfs_fs *fs, int fd, struct stat *st) {
  (void) fs;
  return fs_slfs_fstat(fd, st);
}

off_t cc32xx_vfs_fs_slfs_lseek(struct mgos_vfs_fs *fs, int fd, off_t offset,
                               int whence) {
  (void) fs;
  return fs_slfs_lseek(fd, offset, whence);
}

int cc32xx_vfs_fs_slfs_rename(struct mgos_vfs_fs *fs, const char *src,
                              const char *dst) {
  (void) fs;
  return fs_slfs_rename(src, dst);
}

int cc32xx_vfs_fs_slfs_unlink(struct mgos_vfs_fs *fs, const char *path) {
  (void) fs;
  return fs_slfs_unlink(path);
}

#if MG_ENABLE_DIRECTORY_LISTING
/* SLFS does not support listing files. */
static DIR *cc32xx_vfs_fs_slfs_opendir(struct mgos_vfs_fs *fs,
                                       const char *path) {
  errno = ENOTSUP;
  return NULL;
}

static struct dirent *cc32xx_vfs_fs_slfs_readdir(struct mgos_vfs_fs *fs,
                                                 DIR *dir) {
  errno = ENOTSUP;
  return NULL;
}

static int cc32xx_vfs_fs_slfs_closedir(struct mgos_vfs_fs *fs, DIR *dir) {
  errno = ENOTSUP;
  return -1;
}
#endif /* MG_ENABLE_DIRECTORY_LISTING */

static const struct mgos_vfs_fs_ops cc32xx_vfs_fs_slfs_ops = {
    .mkfs = cc32xx_vfs_fs_slfs_mkfs,
    .mount = cc32xx_vfs_fs_slfs_mount,
    .umount = cc32xx_vfs_fs_slfs_umount,
    .get_space_total = cc32xx_vfs_fs_slfs_get_space_total,
    .get_space_used = cc32xx_vfs_fs_slfs_get_space_used,
    .get_space_free = cc32xx_vfs_fs_slfs_get_space_free,
    .gc = cc32xx_vfs_fs_slfs_gc,
    .open = cc32xx_vfs_fs_slfs_open,
    .close = cc32xx_vfs_fs_slfs_close,
    .read = cc32xx_vfs_fs_slfs_read,
    .write = cc32xx_vfs_fs_slfs_write,
    .stat = cc32xx_vfs_fs_slfs_stat,
    .fstat = cc32xx_vfs_fs_slfs_fstat,
    .lseek = cc32xx_vfs_fs_slfs_lseek,
    .unlink = cc32xx_vfs_fs_slfs_unlink,
    .rename = cc32xx_vfs_fs_slfs_rename,
#if MG_ENABLE_DIRECTORY_LISTING
    .opendir = cc32xx_vfs_fs_slfs_opendir,
    .readdir = cc32xx_vfs_fs_slfs_readdir,
    .closedir = cc32xx_vfs_fs_slfs_closedir,
#endif
};

bool cc32xx_vfs_fs_slfs_register_type(void) {
  return mgos_vfs_fs_register_type(MGOS_VFS_FS_TYPE_SLFS,
                                   &cc32xx_vfs_fs_slfs_ops);
}
