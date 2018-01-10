/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

/*
 * VFS subsystem multiplexes calls to libc file API methods such as open,
 * read, write and close between (potentially) several filesystems attached
 * at different mount points.
 * A filesystem is backed by a device which supports block reads and writes.
 */

#ifndef CS_FW_SRC_MGOS_VFS_H_
#define CS_FW_SRC_MGOS_VFS_H_

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

#include "common/platform.h"
#include "common/queue.h"

#include "mgos_vfs_dev.h"

#ifdef CS_MMAP
#if CS_PLATFORM == CS_P_ESP32
#include "fw/platforms/esp32/src/esp32_fs.h"
#include "fw/platforms/esp32/src/esp32_mmap.h"
#elif CS_PLATFORM == CS_P_ESP8266
#include "fw/platforms/esp8266/src/esp_fs.h"
#include "fw/platforms/esp8266/src/esp8266_mmap.h"
#endif
#endif /* CS_MMAP */

/* Convert virtual fd to filesystem-specific fd */
#define MGOS_VFS_VFD_TO_FS_FD(vfd) ((vfd) &0xff)

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

#ifdef CS_MMAP
/*
 * Platform-dependent header should define the following macros:
 *
 * - MMAP_BASE: base address for mmapped points; e.g. ((void *) 0x10000000)
 * - MMAP_END:  end address for mmapped points; e.g. ((void *) 0x20000000)
 *
 * So with the example values given above, the range 0x10000000 - 0x20000000 is
 * used for all mmapped areas. We need to partition it further, by choosing the
 * optimal tradeoff between the max number of mmapped areas and the max size
 * of the mmapped area. Within the example range, we have 28 bits, and we
 * need to define two more macros which will define how these bits are used:
 *
 * - MMAP_ADDR_BITS: how many bits are used for the address within each
 *   mmapped area;
 * - MMAP_NUM_BITS: how many bits are used for the number of mmapped area.
 */

#define MMAP_NUM_MASK ((1 << MMAP_NUM_BITS) - 1)
#define MMAP_ADDR_MASK ((1 << MMAP_ADDR_BITS) - 1)

/*
 * We need to declare mgos_vfs_mmap_descs in order for MMAP_DESC_FROM_ADDR()
 * and friends to work. We could use a function instead of that, but it's
 * used on a critical path (reading each mmapped byte), so let it be.
 */
extern struct mgos_vfs_mmap_desc *mgos_vfs_mmap_descs;

#define MMAP_DESC_FROM_ADDR(addr)                                 \
  (&mgos_vfs_mmap_descs[(((uintptr_t)(addr)) >> MMAP_ADDR_BITS) & \
                        MMAP_NUM_MASK])
#define MMAP_ADDR_FROM_ADDR(addr) (((uintptr_t)(addr)) & MMAP_ADDR_MASK)

#define MMAP_BASE_FROM_DESC(desc)    \
  ((void *) ((uintptr_t) MMAP_BASE | \
             (((desc) -mgos_vfs_mmap_descs) << MMAP_ADDR_BITS)))

struct mgos_vfs_mmap_desc {
  struct mgos_vfs_fs *fs;
  /* Address at which mmapped data is available */
  void *base;

  /* FS-specific data */
  void *fs_data;
};

void mgos_vfs_mmap_init(void);

/*
 * Returns total number of allocated mmap descriptors (not all of them might be
 * used at the moment)
 */
int mgos_vfs_mmap_descs_cnt(void);

/*
 * Returns mmap descriptor at the given index
 */
struct mgos_vfs_mmap_desc *mgos_vfs_mmap_desc_get(int idx);
#endif /* CS_MMAP */

/* Define DIR and struct dirent. */
#ifndef MGOS_VFS_DEFINE_DIRENT
#define MGOS_VFS_DEFINE_DIRENT 0
#endif

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

/*
 * VFS ops table. Filesystem implementation must provide all of these.
 * Operatiosn that are not supported should return false or -1.
 */
struct mgos_vfs_fs_ops {
  /* Create a filesystem on the given device. Do not mount. */
  bool (*mkfs)(struct mgos_vfs_fs *fs, const char *opts);
  /* Mount the filesystem found on the given device. */
  bool (*mount)(struct mgos_vfs_fs *fs, const char *opts);
  /* Unmount the filesystem. Release all associated recources,
   * this is the last call to this FS instance. */
  bool (*umount)(struct mgos_vfs_fs *fs);
  /* Return total, used and available space on the filesystem. */
  size_t (*get_space_total)(struct mgos_vfs_fs *fs);
  size_t (*get_space_used)(struct mgos_vfs_fs *fs);
  size_t (*get_space_free)(struct mgos_vfs_fs *fs);
  /* Perform garbage collection, if necessary. */
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

#ifdef CS_MMAP
  int (*mmap)(int vfd, size_t len, struct mgos_vfs_mmap_desc *desc);
  void (*munmap)(struct mgos_vfs_mmap_desc *desc);
  uint8_t (*read_mmapped_byte)(struct mgos_vfs_mmap_desc *desc, uint32_t addr);
#endif /* CS_MMAP */

#if 0 /* These parts of the libc API are not supported for now. */
  int (*link)(struct mgos_vfs_fs *fs, const char *n1, const char *n2);
  long (*telldir)(struct mgos_vfs_fs *fs, DIR *pdir);
  void (*seekdir)(struct mgos_vfs_fs *fs, DIR *pdir, long offset);
  int (*mkdir)(struct mgos_vfs_fs *fs, const char *name, mode_t mode);
  int (*rmdir)(struct mgos_vfs_fs *fs, const char *name);
#endif
};

/* Register fielsystem type and make it available for use in mkfs and mount. */
bool mgos_vfs_fs_register_type(const char *type,
                               const struct mgos_vfs_fs_ops *ops);

/*
 * Create a filesystem.
 * First a device is opened with given type and options and then filesystem
 * is created on it. Device and filesystem types must've been previosuly
 * registered and options have device and filesystem-specific format
 * and usually are JSON objects.
 */
bool mgos_vfs_mkfs(const char *dev_type, const char *dev_opts,
                   const char *fs_type, const char *fs_opts);

/*
 * Mount a filesystem.
 * First a device is opened with given type and options and then filesystem
 * is mounted from it and attached to the VFS at a given path.
 * Path must start with a "/" and consist of one component, e.g. "/mnt".
 * Nested mounts are not currently supported, so "/mnt/foo" is not ok.
 * Device and filesystem types must've been previosly registered and options
 * have device and filesystem-specific format and usually are JSON objects.
 */
bool mgos_vfs_mount(const char *path, const char *dev_type,
                    const char *dev_opts, const char *fs_type,
                    const char *fs_opts);

/*
 * Unmount a previously mounted filesystem.
 * Only filesystems with no open files can be unmounted.
 */
bool mgos_vfs_umount(const char *path);

/*
 * Unmount all the filesystems, regardless of open files.
 * Done only on reboot.
 */
void mgos_vfs_umount_all(void);

/*
 * Perform GC of a filesystem at the specified mountpoint.
 */
bool mgos_vfs_gc(const char *path);

/*
 * Platform implementation must ensure that paths prefixed with "path" are
 * routed to "fs" and file descriptors are translated appropriately.
 */
bool mgos_vfs_hal_mount(const char *path, struct mgos_vfs_fs *fs);

/*
 * Clean up path, see realpath(3).
 */
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

/* If this is enabled, it also defines open, read, write -> mog_vfs_* shims. */
#ifndef MGOS_VFS_DEFINE_LIBC_API
#define MGOS_VFS_DEFINE_LIBC_API 0
#endif

/* Define _open_r, _read_r, _write_r -> mgos_vfs_* shims */
#ifndef MGOS_VFS_DEFINE_LIBC_REENT_API
#define MGOS_VFS_DEFINE_LIBC_REENT_API 0
#endif

/* Define opendir, readdir and closedir. */
#ifndef MGOS_VFS_DEFINE_LIBC_DIR_API
#define MGOS_VFS_DEFINE_LIBC_DIR_API 0
#endif

/* Define the _r variants of the open/read/closedir API. */
#ifndef MGOS_VFS_DEFINE_LIBC_REENT_DIR_API
#define MGOS_VFS_DEFINE_LIBC_REENT_DIR_API 0
#endif

/* Define mmap, munmap. */
#ifndef MGOS_VFS_DEFINE_LIBC_MMAP_API
#if defined(CS_MMAP)
#define MGOS_VFS_DEFINE_LIBC_MMAP_API 1
#else /* CS_MMAP */
#define MGOS_VFS_DEFINE_LIBC_MMAP_API 0
#endif /* CS_MMAP */
#endif

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_SRC_MGOS_VFS_H_ */
