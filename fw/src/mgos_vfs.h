/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_VFS_H_
#define CS_FW_SRC_MGOS_VFS_H_

#include <stddef.h>
#include <stdint.h>

#include "common/platform.h"

#include "fw/src/mgos_vfs_dev.h"

#ifndef MGOS_VFS_DEFINE_LIBC_API
#define MGOS_VFS_DEFINE_LIBC_API 0
#endif

#ifndef MGOS_VFS_DEFINE_LIBC_REENT_API
#define MGOS_VFS_DEFINE_LIBC_REENT_API 0
#endif

#ifndef MGOS_VFS_DEFINE_DIRENT
#define MGOS_VFS_DEFINE_DIRENT 0
#endif

#ifndef MGOS_VFS_DEFINE_LIBC_DIR_API
#define MGOS_VFS_DEFINE_LIBC_DIR_API 0
#endif

#ifndef MGOS_VFS_DEFINE_LIBC_REENT_DIR_API
#define MGOS_VFS_DEFINE_LIBC_REENT_DIR_API 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct mgos_vfs_fs {
  int refs;
  const char *type;
  const struct mgos_vfs_fs_ops *ops;
  struct mgos_vfs_dev *dev;
  void *fs_data;
};

#if MG_ENABLE_DIRECTORY_LISTING
#if MGOS_VFS_DEFINE_DIRENT
typedef struct {
} DIR;
struct dirent {
  int d_ino;
  char d_name[256];
};
DIR *opendir(const char *path);
struct dirent *readdir(DIR *pdir);
int closedir(DIR *pdir);
#else
#include <dirent.h>
#endif /* MGOS_VFS_DEFINE_DIRENT */
#endif /* MG_ENABLE_DIRECTORY_LISTING */

struct mgos_vfs_fs_ops {
  bool (*mount)(struct mgos_vfs_fs *fs, const char *opts);
  bool (*umount)(struct mgos_vfs_fs *fs);
  size_t (*get_space_total)(struct mgos_vfs_fs *fs);
  size_t (*get_space_used)(struct mgos_vfs_fs *fs);
  size_t (*get_space_free)(struct mgos_vfs_fs *fs);
  bool (*gc)(struct mgos_vfs_fs *fs);
  /* libc API */
  int (*open)(struct mgos_vfs_fs *fs, const char *path, int flags, int mode);
  int (*close)(struct mgos_vfs_fs *fs, int fd);
  ssize_t (*read)(struct mgos_vfs_fs *fs, int fd, void *dst, size_t len);
  ssize_t (*write)(struct mgos_vfs_fs *fs, int fd, const void *src, size_t len);
  int (*stat)(struct mgos_vfs_fs *fs, const char *path, struct stat *st);
  int (*fstat)(struct mgos_vfs_fs *fs, int fd, struct stat *st);
  off_t (*lseek)(struct mgos_vfs_fs *fs, int fd, off_t offset, int whence);
  int (*unlink)(struct mgos_vfs_fs *fs, const char *path);
  int (*rename)(struct mgos_vfs_fs *fs, const char *src, const char *dst);
#if MG_ENABLE_DIRECTORY_LISTING
  DIR *(*opendir)(struct mgos_vfs_fs *fs, const char *path);
  struct dirent *(*readdir)(struct mgos_vfs_fs *fs, DIR *pdir);
  int (*closedir)(struct mgos_vfs_fs *fs, DIR *pdir);
#endif
#if 0 /* These parts of the libc API are not supported for now. */
  int (*link)(struct mgos_vfs_fs *fs, const char *n1, const char *n2);
  long (*telldir)(struct mgos_vfs_fs *fs, DIR *pdir);
  void (*seekdir)(struct mgos_vfs_fs *fs, DIR *pdir, long offset);
  int (*mkdir)(struct mgos_vfs_fs *fs, const char *name, mode_t mode);
  int (*rmdir)(struct mgos_vfs_fs *fs, const char *name);
#endif
};

bool mgos_vfs_fs_register_type(const char *type,
                               const struct mgos_vfs_fs_ops *ops);

bool mgos_vfs_mount(const char *path, const char *dev_type,
                    const char *dev_opts, const char *fs_type,
                    const char *fs_opts);

bool mgos_vfs_umount(const char *path);

void mgos_vfs_umount_all(void);

bool mgos_vfs_gc(const char *path);

/*
 * Platform implementation must ensure that paths prefixed with "path" are
 * routed to "fs" and file descriptors are translated appropriately.
 */
bool mgos_vfs_hal_mount(const char *path, struct mgos_vfs_fs *fs);

char *mgos_realpath(const char *path, char *resolved_path);

/* libc API */
int mgos_vfs_open(const char *filename, int flags, int mode);
int mgos_vfs_close(int vfd);
ssize_t mgos_vfs_read(int vfd, void *dst, size_t len);
ssize_t mgos_vfs_write(int vfd, const void *src, size_t len);
int mgos_vfs_stat(const char *path, struct stat *st);
int mgos_vfs_fstat(int vfd, struct stat *st);
off_t mgos_vfs_lseek(int vfd, off_t offset, int whence);
int mgos_vfs_unlink(const char *path);
int mgos_vfs_rename(const char *src, const char *dst);
#if MG_ENABLE_DIRECTORY_LISTING
DIR *mgos_vfs_opendir(const char *path);
struct dirent *mgos_vfs_readdir(DIR *pdir);
int mgos_vfs_closedir(DIR *pdir);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_SRC_MGOS_VFS_H_ */
