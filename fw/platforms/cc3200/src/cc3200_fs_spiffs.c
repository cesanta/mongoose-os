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
  fs_lock(m);
  int ret = spiffs_vfs_open(&m->fs, pathname, flags, mode);
  fs_unlock(m);
  return ret;
}

int fs_spiffs_close(int fd) {
  struct mount_info *m = &s_fsm;
  fs_lock(m);
  int ret = spiffs_vfs_close(&m->fs, fd);
  fs_unlock(m);
  return ret;
}

ssize_t fs_spiffs_read(int fd, void *buf, size_t count) {
  struct mount_info *m = &s_fsm;
  fs_lock(m);
  ssize_t ret = spiffs_vfs_read(&m->fs, fd, buf, count);
  fs_unlock(m);
  return ret;
}

ssize_t fs_spiffs_write(int fd, const void *buf, size_t count) {
  struct mount_info *m = &s_fsm;
  fs_lock(m);
  ssize_t ret = spiffs_vfs_write(&m->fs, fd, buf, count);
  fs_unlock(m);
  return ret;
}

int fs_spiffs_stat(const char *pathname, struct stat *s) {
  struct mount_info *m = &s_fsm;
  fs_lock(m);
  int ret = spiffs_vfs_stat(&m->fs, pathname, s);
  fs_unlock(m);
  return ret;
}

int fs_spiffs_fstat(int fd, struct stat *s) {
  struct mount_info *m = &s_fsm;
  fs_lock(m);
  int ret = spiffs_vfs_fstat(&m->fs, fd, s);
  fs_unlock(m);
  return ret;
}

off_t fs_spiffs_lseek(int fd, off_t offset, int whence) {
  struct mount_info *m = &s_fsm;
  fs_lock(m);
  off_t ret = spiffs_vfs_lseek(&m->fs, fd, offset, whence);
  fs_unlock(m);
  return ret;
}

int fs_spiffs_rename(const char *from, const char *to) {
  struct mount_info *m = &s_fsm;
  fs_lock(m);
  int ret = spiffs_vfs_rename(&m->fs, from, to);
  fs_unlock(m);
  return ret;
}

int fs_spiffs_unlink(const char *filename) {
  struct mount_info *m = &s_fsm;
  fs_lock(m);
  int ret = spiffs_vfs_unlink(&m->fs, filename);
  fs_unlock(m);
  return ret;
}

DIR *fs_spiffs_opendir(const char *dir_name) {
  struct mount_info *m = &s_fsm;
  fs_lock(m);
  DIR *ret = spiffs_vfs_opendir(&m->fs, dir_name);
  fs_unlock(m);
  return ret;
}

struct dirent *fs_spiffs_readdir(DIR *dir) {
  struct mount_info *m = &s_fsm;
  fs_lock(m);
  struct dirent *ret = spiffs_vfs_readdir(&m->fs, dir);
  fs_unlock(m);
  return ret;
}

int fs_spiffs_closedir(DIR *dir) {
  struct mount_info *m = &s_fsm;
  fs_lock(m);
  int ret = spiffs_vfs_closedir(&m->fs, dir);
  fs_unlock(m);
  return ret;
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

void cc3200_fs_flush_locked(void) {
  struct mount_info *m = &s_fsm;
  /*
   * If container is open for writing and we've been idle for a while,
   * close it to avoid the dreaded SL_FS_FILE_HAS_NOT_BEEN_CLOSE_CORRECTLY.
   */

  if (!m->rw) return;
  fs_close_container(m);
}

void cc3200_fs_lock(void) {
  fs_lock(&s_fsm);
}

void cc3200_fs_unlock(void) {
  fs_unlock(&s_fsm);
}

void cc3200_fs_flush(void) {
  cc3200_fs_lock();
  cc3200_fs_flush_locked();
  cc3200_fs_unlock();
}

static void cc3200_fs_flush_if_inactive(void *arg) {
  struct mount_info *m = &s_fsm;
  if (!m->rw) return; /* Quick check without lock. */
  double now = mg_time();
  fs_lock(m);
  if (now - m->last_write > 0.5) {
    cc3200_fs_flush_locked();
  }
  fs_unlock(m);
}

int cc3200_fs_init(const char *container_prefix) {
  mgos_add_poll_cb(cc3200_fs_flush_if_inactive, NULL);
  return fs_mount(container_prefix, &s_fsm);
}

void cc3200_fs_umount() {
  struct mount_info *m = &s_fsm;
  fs_lock(m);
  fs_umount(m);
  /* Destroys the lock, no need to unlock. */
}
