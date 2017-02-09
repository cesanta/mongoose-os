/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/* LIBC interface to SPIFFS inside the container. */

#include "cc3200_fs_spiffs.h"
#include "cc3200_fs_spiffs_container.h"

#include <errno.h>
#include <stdlib.h>

#include <common/spiffs/spiffs_nucleus.h>
#include <common/spiffs/spiffs_vfs.h>

#include "fw/src/mgos_mongoose.h"

struct mount_info s_fsm;

size_t mgos_get_fs_size(void) {
  u32_t total, used;
  if (SPIFFS_info(&s_fsm.fs, &total, &used) != SPIFFS_OK) return 0;
  return total;
}

size_t mgos_get_free_fs_size(void) {
  u32_t total, used;
  if (SPIFFS_info(&s_fsm.fs, &total, &used) != SPIFFS_OK) return 0;
  return total - used;
}

int fs_spiffs_open(const char *pathname, int flags, mode_t mode) {
  struct mount_info *m = &s_fsm;
  if (!s_fsm.valid) return set_errno(ENXIO);
  return spiffs_vfs_open(&m->fs, pathname, flags, mode);
}

int fs_spiffs_close(int fd) {
  struct mount_info *m = &s_fsm;
  if (!m->valid) return set_errno(ENXIO);
  return spiffs_vfs_close(&m->fs, fd);
}

ssize_t fs_spiffs_read(int fd, void *buf, size_t count) {
  struct mount_info *m = &s_fsm;
  if (!m->valid) return set_errno(ENXIO);
  return spiffs_vfs_read(&m->fs, fd, buf, count);
}

ssize_t fs_spiffs_write(int fd, const void *buf, size_t count) {
  struct mount_info *m = &s_fsm;
  if (!m->valid) return set_errno(ENXIO);
  return spiffs_vfs_write(&m->fs, fd, buf, count);
}

int fs_spiffs_stat(const char *pathname, struct stat *s) {
  struct mount_info *m = &s_fsm;
  if (!m->valid) return set_errno(ENXIO);
  return spiffs_vfs_stat(&m->fs, pathname, s);
}

int fs_spiffs_fstat(int fd, struct stat *s) {
  struct mount_info *m = &s_fsm;
  if (!m->valid) return set_errno(ENXIO);
  return spiffs_vfs_fstat(&m->fs, fd, s);
}

off_t fs_spiffs_lseek(int fd, off_t offset, int whence) {
  struct mount_info *m = &s_fsm;
  if (!m->valid) return set_errno(ENXIO);
  return spiffs_vfs_lseek(&m->fs, fd, offset, whence);
}

int fs_spiffs_rename(const char *from, const char *to) {
  struct mount_info *m = &s_fsm;
  if (!m->valid) return set_errno(ENXIO);
  return spiffs_vfs_rename(&m->fs, from, to);
}

int fs_spiffs_unlink(const char *filename) {
  struct mount_info *m = &s_fsm;
  if (!m->valid) return set_errno(ENXIO);
  return spiffs_vfs_unlink(&m->fs, filename);
}

DIR *fs_spiffs_opendir(const char *dir_name) {
  struct mount_info *m = &s_fsm;
  if (!m->valid) {
    set_errno(ENXIO);
    return NULL;
  }
  return spiffs_vfs_opendir(&m->fs, dir_name);
}

struct dirent *fs_spiffs_readdir(DIR *dir) {
  struct mount_info *m = &s_fsm;
  if (!m->valid) {
    set_errno(ENXIO);
    return NULL;
  }
  return spiffs_vfs_readdir(&m->fs, dir);
}

int fs_spiffs_closedir(DIR *dir) {
  struct mount_info *m = &s_fsm;
  if (!m->valid) return set_errno(ENXIO);
  return spiffs_vfs_closedir(&m->fs, dir);
}

/* SPIFFs doesn't support directory operations */
int fs_spiffs_rmdir(const char *path) {
  (void) path;
  return ENOTDIR;
}

int fs_spiffs_mkdir(const char *path, mode_t mode) {
  (void) path;
  (void) mode;
  /* for spiffs supports only root dir, which comes from mongoose as '.' */
  return (strlen(path) == 1 && *path == '.') ? 0 : ENOTDIR;
}

void cc3200_fs_flush(void) {
  struct mount_info *m = &s_fsm;
  /*
   * If container is open for writing and we've been idle for a while,
   * close it to avoid the dreaded SL_FS_FILE_HAS_NOT_BEEN_CLOSE_CORRECTLY.
   */
  if (!(m->valid && m->rw)) return;
  fs_close_container(m);
}

static void cc3200_fs_flush_if_inactive(void *arg) {
  struct mount_info *m = &s_fsm;
  if (!(m->valid && m->rw)) return;
  double now = mg_time();
  if (now - m->last_write > 0.5) {
    cc3200_fs_flush();
  }
}

int cc3200_fs_init(const char *container_prefix) {
  mgos_add_poll_cb(cc3200_fs_flush_if_inactive, NULL);
  return fs_mount(container_prefix, &s_fsm);
}

void cc3200_fs_umount() {
  if (s_fsm.valid) fs_umount(&s_fsm);
}
