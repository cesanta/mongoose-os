/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "mgos_vfs_internal.h"

#include <string.h>
#if MGOS_VFS_DEFINE_LIBC_REENT_API
#include <sys/reent.h>
#endif

#include "mongoose/mongoose.h" /* For MG_MAX_PATH */

#include "common/cs_dbg.h"
#include "common/queue.h"

#include "mgos_debug.h"
#include "mgos_hal.h"

#ifdef CS_MMAP
#include <sys/mman.h>
#endif /* CS_MMAP */

#define MAKE_VFD(mount_id, fs_fd) (((mount_id) << 8) | ((fs_fd) &0xff))
#define MOUNT_ID_FROM_VFD(fd) (((fd) >> 8) & 0xff)

struct mgos_vfs_fs_entry {
  const char *type;
  const struct mgos_vfs_fs_ops *ops;
  SLIST_ENTRY(mgos_vfs_fs_entry) next;
};

static SLIST_HEAD(s_fses,
                  mgos_vfs_fs_entry) s_fses = SLIST_HEAD_INITIALIZER(s_fses);

struct mgos_vfs_mount_entry {
  int mount_id;
  char *prefix;
  size_t prefix_len;
  struct mgos_vfs_fs *fs;
  SLIST_ENTRY(mgos_vfs_mount_entry) next;
};

SLIST_HEAD(s_mounts,
           mgos_vfs_mount_entry) s_mounts = SLIST_HEAD_INITIALIZER(s_mounts);

bool mgos_vfs_fs_register_type(const char *type,
                               const struct mgos_vfs_fs_ops *ops) {
  if (ops->mkfs == NULL || ops->mount == NULL || ops->umount == NULL ||
      ops->get_space_total == NULL || ops->get_space_used == NULL ||
      ops->get_space_free == NULL || ops->gc == NULL) {
    LOG(LL_ERROR, ("%s: not all required methods are implemented", type));
    abort();
  }
  struct mgos_vfs_fs_entry *fe =
      (struct mgos_vfs_fs_entry *) calloc(1, sizeof(*fe));
  if (fe == NULL) return false;
  fe->type = type;
  fe->ops = ops;
  SLIST_INSERT_HEAD(&s_fses, fe, next);
  return true;
}

static inline void mgos_vfs_lock(void) {
  mgos_lock();
}

static inline void mgos_vfs_unlock(void) {
  mgos_unlock();
}

bool mgos_vfs_mkfs(const char *dev_type, const char *dev_opts,
                   const char *fs_type, const char *fs_opts) {
  struct mgos_vfs_dev *dev = NULL;
  struct mgos_vfs_fs_entry *fe = NULL;
  if (fs_type == NULL) return NULL;
  SLIST_FOREACH(fe, &s_fses, next) {
    if (strcmp(fs_type, fe->type) == 0) {
      bool ret = false;
      if (fs_opts == NULL) fs_opts = "";
      if (dev_type != NULL) {
        dev = mgos_vfs_dev_open(dev_type, dev_opts);
        if (dev == NULL) return false;
      }
      struct mgos_vfs_fs fs = {.type = fe->type, .ops = fe->ops, .dev = dev};
      LOG(LL_INFO, ("Create %s (dev %p, opts %s)", fs_type, dev, fs_opts));
      ret = (fs.ops->mkfs(&fs, fs_opts));
      if (!ret) {
        LOG(LL_INFO, ("FS %s %s: create failed", fs_type, fs_opts));
      }
      if (dev != NULL) mgos_vfs_dev_close(dev);
      return ret;
    }
  };
  LOG(LL_INFO, ("Unknown FS type %s", fs_type));
  return false;
}

bool mgos_vfs_mount(const char *path, const char *dev_type,
                    const char *dev_opts, const char *fs_type,
                    const char *fs_opts) {
  struct mgos_vfs_dev *dev = NULL;
  struct mgos_vfs_fs *fs = NULL;
  struct mgos_vfs_fs_entry *fe = NULL;
  if (path == NULL || path[0] != DIRSEP || fs_type == NULL) {
    return NULL;
  }
  SLIST_FOREACH(fe, &s_fses, next) {
    if (strcmp(fs_type, fe->type) == 0) {
      if (fs_opts == NULL) fs_opts = "";
      if (dev_type != NULL) {
        dev = mgos_vfs_dev_open(dev_type, dev_opts);
        if (dev == NULL) return false;
        dev->refs++;
      }
      fs = (struct mgos_vfs_fs *) calloc(1, sizeof(*fs));
      if (fs == NULL) break;
      fs->type = fe->type;
      fs->ops = fe->ops;
      fs->dev = dev;
      LOG(LL_INFO, ("Mount %s @ %s (dev %p, opts %s) -> %p", fs_type, path, dev,
                    fs_opts, fs));
      if (fs->ops->mount(fs, fs_opts)) {
        LOG(LL_INFO, ("%s: size %u, used: %u, free: %u", path,
                      (unsigned int) fs->ops->get_space_total(fs),
                      (unsigned int) fs->ops->get_space_used(fs),
                      (unsigned int) fs->ops->get_space_free(fs)));
        mgos_vfs_hal_mount(path, fs);
        return true;
      } else {
        free(fs);
        LOG(LL_INFO, ("FS %s %s: mount failed", fs_type, fs_opts));
        if (dev != NULL) mgos_vfs_dev_close(dev);
        return false;
      }
    }
  };
  if (fs == NULL) {
    LOG(LL_INFO, ("Unknown FS type %s", fs_type));
  }
  return false;
}

char *mgos_realpath(const char *path, char *resolved_path) {
  const char *p = path;
  bool alloced = false;
  char *rp = resolved_path;
  size_t rpl = 0;
  /* Our paths cannot grow by more than 1 char: when / (cwd) is prepended. */
  size_t rps = strlen(path) + 1 + 1 /* NUL */;
  if (rps > MG_MAX_PATH) return NULL;
  if (resolved_path == NULL) {
    rp = resolved_path = malloc(rps);
    alloced = true;
  }
  if (rp == NULL) return NULL;
  /* If path is relative (does not start with '/' or starts with './'),
   * assume cwd is '/'. */
  if (path[0] != DIRSEP) {
    *rp++ = '/';
    rpl = 1;
  }
  if (path[0] == '.' && path[1] == DIRSEP) {
    p = path + 1;
  }
  while (*p != '\0') {
    *rp++ = *p;
    if (*p == DIRSEP) {
      while (*p == DIRSEP) p++;
    } else {
      p++;
    }
    rpl++;
    if (rpl >= rps) {
      if (alloced) free(resolved_path);
      resolved_path = NULL;
      break;
    }
  }
  if (resolved_path != NULL) {
    if (rpl > 1 && resolved_path[rpl - 1] == DIRSEP) {
      rpl--;
    }
    resolved_path[rpl] = '\0';
  }
  return resolved_path;
}

static struct mgos_vfs_mount_entry *find_mount_by_path(const char *path,
                                                       char **fs_path) {
  struct mgos_vfs_mount_entry *me = NULL;
  size_t prefix_len = 0;
  char *real_path = mgos_realpath(path, NULL), *p;
  if (fs_path != NULL) *fs_path = NULL;
  if (real_path == NULL) goto out;
  for (prefix_len = 1, p = real_path + 1; *p != DIRSEP && *p != '\0'; p++) {
  }
  if (*p == DIRSEP) prefix_len = p - real_path;
  mgos_vfs_lock();
  SLIST_FOREACH(me, &s_mounts, next) {
    if (me->prefix_len == prefix_len &&
        strncmp(real_path, me->prefix, prefix_len) == 0) {
      me->fs->refs++;
      break;
    }
    /* Full match */
    if (strcmp(real_path, me->prefix) == 0) {
      prefix_len = me->prefix_len;
      me->fs->refs++;
      break;
    }
  }
  mgos_vfs_unlock();
out:
  LOG(LL_DEBUG, ("%s -> %s pl %u -> %d %p", path, (real_path ? real_path : ""),
                 (unsigned int) prefix_len, (me ? me->mount_id : -1),
                 (me ? me->fs : NULL)));
  if (me != NULL && fs_path != NULL) {
    *fs_path = real_path;
    p = real_path + prefix_len;
    if (*p == DIRSEP) p++;
    while (*p != '\0') *real_path++ = *p++;
    *real_path = '\0';
  } else {
    free(real_path);
  }
  return me;
}

static struct mgos_vfs_mount_entry *find_mount_by_mount_id(int id) {
  struct mgos_vfs_mount_entry *me = NULL;
  mgos_vfs_lock();
  SLIST_FOREACH(me, &s_mounts, next) {
    if (me->mount_id == id) {
      /* We have an open fd already, do not increment refs */
      break;
    }
  }
  mgos_vfs_unlock();
  return me;
}

static struct mgos_vfs_mount_entry *find_mount_by_vfd(int vfd) {
  return find_mount_by_mount_id(MOUNT_ID_FROM_VFD(vfd));
}

static int find_free_mount_id(void) {
  static int s_last_id = 0;
  int id = s_last_id;
  do {
    id = (id + 1) % 0xff;
    /* Zero is special, do not use it. */
    if (id == 0) continue;
    /* Make sure it's not used. */
  } while (find_mount_by_mount_id(id) != NULL);
  s_last_id = id;
  return id;
}

bool mgos_vfs_hal_mount(const char *path, struct mgos_vfs_fs *fs) {
  struct mgos_vfs_mount_entry *me =
      (struct mgos_vfs_mount_entry *) calloc(1, sizeof(*me));
  if (me == NULL) return false;
  mgos_vfs_lock();
  me->mount_id = find_free_mount_id();
  me->prefix = strdup(path);
  me->prefix_len = strlen(path);
  me->fs = fs;
  SLIST_INSERT_HEAD(&s_mounts, me, next);
  mgos_vfs_unlock();
  return true;
}

int mgos_vfs_open(const char *path, int flags, int mode) {
  int fs_fd = -1, vfd = -1;
  char *fs_path = NULL;
  struct mgos_vfs_mount_entry *me = find_mount_by_path(path, &fs_path);
  struct mgos_vfs_fs *fs = NULL;
  mgos_vfs_lock();
  if (me == NULL) {
    errno = ENOENT;
    goto out;
  }
  fs = me->fs;
  fs_fd = fs->ops->open(fs, fs_path, flags, mode);
  if (fs_fd >= 0) {
    if (fs_fd <= 0xff) {
      vfd = MAKE_VFD(me->mount_id, fs_fd);
    } else {
      errno = EBADF;
    }
  } else {
    me->fs->refs--;
    vfd = fs_fd;
  }
out:
  mgos_vfs_unlock();
  LOG(LL_DEBUG,
      ("%s 0x%x 0x%x => %p %s %d => %d (refs %d)", path, flags, mode, fs,
       (fs_path ? fs_path : ""), fs_fd, vfd, (me ? me->fs->refs : -1)));
  free(fs_path);
  return vfd;
}
#if MGOS_VFS_DEFINE_LIBC_API
int open(const char *filename, int flags, int mode) {
  return mgos_vfs_open(filename, flags, mode);
}
#endif
#if MGOS_VFS_DEFINE_LIBC_REENT_API
int _open_r(struct _reent *r, const char *filename, int flags, int mode) {
  (void) r;
  return mgos_vfs_open(filename, flags, mode);
}
#endif

int mgos_vfs_close(int vfd) {
  int ret = -1, fs_fd = MGOS_VFS_VFD_TO_FS_FD(vfd);
  struct mgos_vfs_mount_entry *me = find_mount_by_vfd(vfd);
  struct mgos_vfs_fs *fs = NULL;
  mgos_vfs_lock();
  if (me == NULL) {
    errno = EBADF;
    goto out;
  }
  fs = me->fs;
  ret = fs->ops->close(fs, fs_fd);
out:
  if (ret == 0) {
    me->fs->refs--;
  }
  mgos_vfs_unlock();
  LOG(LL_DEBUG, ("%d => %p:%d => %d (refs %d)", vfd, fs, fs_fd, ret,
                 (me ? me->fs->refs : -1)));
  return ret;
}
#if MGOS_VFS_DEFINE_LIBC_API
int close(int vfd) {
  return mgos_vfs_close(vfd);
}
#endif
#if MGOS_VFS_DEFINE_LIBC_REENT_API
int _close_r(struct _reent *r, int vfd) {
  (void) r;
  return mgos_vfs_close(vfd);
}
#endif

ssize_t mgos_vfs_read(int vfd, void *dst, size_t len) {
  int ret = -1, fs_fd = MGOS_VFS_VFD_TO_FS_FD(vfd);
  struct mgos_vfs_mount_entry *me = find_mount_by_vfd(vfd);
  struct mgos_vfs_fs *fs = NULL;
  if (me == NULL) {
    errno = EBADF;
    goto out;
  }
  fs = me->fs;
  ret = fs->ops->read(fs, fs_fd, dst, len);
out:
  LOG(LL_VERBOSE_DEBUG,
      ("%d %u => %p:%d => %d", vfd, (unsigned int) len, fs, fs_fd, ret));
  return ret;
}
#if MGOS_VFS_DEFINE_LIBC_API
ssize_t read(int vfd, void *dst, size_t len) {
  return mgos_vfs_read(vfd, dst, len);
}
#endif
#if MGOS_VFS_DEFINE_LIBC_REENT_API
ssize_t _read_r(struct _reent *r, int vfd, void *dst, size_t len) {
  (void) r;
  return mgos_vfs_read(vfd, dst, len);
}
#endif

ssize_t mgos_vfs_write(int vfd, const void *src, size_t len) {
  ssize_t ret = -1;
  int fs_fd = MGOS_VFS_VFD_TO_FS_FD(vfd);
  int mid = MOUNT_ID_FROM_VFD(vfd);
  struct mgos_vfs_mount_entry *me = NULL;
  struct mgos_vfs_fs *fs = NULL;
  /* Handle stdout and stderr. */
  if (mid == 0) {
    if (fs_fd == 1 || fs_fd == 2) {
      mgos_debug_write(fs_fd, src, len);
      return len;
    }
    errno = EBADF;
    goto out;
  }
  me = find_mount_by_vfd(vfd);
  if (me == NULL) {
    errno = EBADF;
    goto out;
  }
  fs = me->fs;
  ret = fs->ops->write(fs, fs_fd, src, len);
out:
  LOG(LL_DEBUG,
      ("%d %u => %p:%d => %d", vfd, (unsigned int) len, fs, fs_fd, (int) ret));
  return ret;
}
#if MGOS_VFS_DEFINE_LIBC_API
ssize_t write(int vfd, const void *src, size_t len) {
  return mgos_vfs_write(vfd, src, len);
}
#endif
#if MGOS_VFS_DEFINE_LIBC_REENT_API
ssize_t _write_r(struct _reent *r, int vfd, const void *buf, size_t len) {
  (void) r;
  return mgos_vfs_write(vfd, buf, len);
}
#endif

int mgos_vfs_stat(const char *path, struct stat *st) {
  int ret = -1;
  char *fs_path = NULL;
  struct mgos_vfs_mount_entry *me = find_mount_by_path(path, &fs_path);
  struct mgos_vfs_fs *fs = NULL;
  mgos_vfs_lock();
  if (me == NULL) {
    errno = ENOENT;
    goto out;
  }
  fs = me->fs;
  ret = fs->ops->stat(fs, fs_path, st);
out:
  if (me != NULL) me->fs->refs--;
  mgos_vfs_unlock();
  LOG(LL_DEBUG,
      ("%s => %p %s => %d (size %d)", path, fs, (fs_path ? fs_path : ""), ret,
       (int) (ret == 0 ? st->st_size : 0)));
  free(fs_path);
  return ret;
}
#if MGOS_VFS_DEFINE_LIBC_API
int stat(const char *path, struct stat *st) {
  return mgos_vfs_stat(path, st);
}
#endif
#if MGOS_VFS_DEFINE_LIBC_REENT_API
int _stat_r(struct _reent *r, const char *path, struct stat *st) {
  (void) r;
  return mgos_vfs_stat(path, st);
}
#endif

int mgos_vfs_fstat(int vfd, struct stat *st) {
  int ret = -1, fs_fd = MGOS_VFS_VFD_TO_FS_FD(vfd);
  struct mgos_vfs_mount_entry *me = find_mount_by_vfd(vfd);
  struct mgos_vfs_fs *fs = NULL;
  if (me == NULL) {
    errno = ENOENT;
    goto out;
  }
  fs = me->fs;
  ret = fs->ops->fstat(fs, fs_fd, st);
out:
  LOG(LL_DEBUG, ("%d => %p:%d => %d (size %d)", vfd, fs, fs_fd, ret,
                 (int) (ret == 0 ? st->st_size : 0)));
  return ret;
}
#if MGOS_VFS_DEFINE_LIBC_API
int fstat(int vfd, struct stat *st) {
  return mgos_vfs_fstat(vfd, st);
}
#endif
#if MGOS_VFS_DEFINE_LIBC_REENT_API
int _fstat_r(struct _reent *r, int vfd, struct stat *st) {
  (void) r;
  return mgos_vfs_fstat(vfd, st);
}
#endif

off_t mgos_vfs_lseek(int vfd, off_t offset, int whence) {
  off_t ret = -1;
  int fs_fd = MGOS_VFS_VFD_TO_FS_FD(vfd);
  struct mgos_vfs_mount_entry *me = find_mount_by_vfd(vfd);
  struct mgos_vfs_fs *fs = NULL;
  if (me == NULL) {
    errno = EBADF;
    goto out;
  }
  fs = me->fs;
  ret = fs->ops->lseek(fs, fs_fd, offset, whence);
out:
  LOG(LL_DEBUG, ("%d %ld %d => %p:%d => %ld", vfd, (long int) offset, whence,
                 fs, fs_fd, (long int) ret));
  return ret;
}
#if MGOS_VFS_DEFINE_LIBC_API
off_t lseek(int vfd, off_t offset, int whence) {
  return mgos_vfs_lseek(vfd, offset, whence);
}
#endif
#if MGOS_VFS_DEFINE_LIBC_REENT_API
off_t _lseek_r(struct _reent *r, int vfd, off_t offset, int whence) {
  (void) r;
  return mgos_vfs_lseek(vfd, offset, whence);
}
#endif

int mgos_vfs_unlink(const char *path) {
  int ret = -1;
  char *fs_path = NULL;
  struct mgos_vfs_mount_entry *me = find_mount_by_path(path, &fs_path);
  struct mgos_vfs_fs *fs = NULL;
  mgos_vfs_lock();
  if (me == NULL) {
    errno = ENOENT;
    goto out;
  }
  fs = me->fs;
  ret = fs->ops->unlink(fs, fs_path);
out:
  if (me != NULL) me->fs->refs--;
  mgos_vfs_unlock();
  LOG(LL_DEBUG, ("%s => %p %s => %d", path, fs, (fs_path ? fs_path : ""), ret));
  free(fs_path);
  return ret;
}
#if MGOS_VFS_DEFINE_LIBC_API
int unlink(const char *path) {
  return mgos_vfs_unlink(path);
}
#endif
#if MGOS_VFS_DEFINE_LIBC_REENT_API
int _unlink_r(struct _reent *r, const char *path) {
  (void) r;
  return mgos_vfs_unlink(path);
}
#endif

int mgos_vfs_rename(const char *src, const char *dst) {
  int ret = -1;
  struct mgos_vfs_fs *fs = NULL;
  char *fs_src = NULL, *fs_dst = NULL;
  struct mgos_vfs_mount_entry *me = find_mount_by_path(src, &fs_src);
  struct mgos_vfs_mount_entry *me_dst = find_mount_by_path(dst, &fs_dst);
  mgos_vfs_lock();
  if (me == NULL || me_dst == NULL) {
    errno = ENODEV;
    goto out;
  } else if (me != me_dst) {
    me_dst->fs->refs--;
    errno = EXDEV;
    goto out;
  }
  fs = me->fs;
  ret = fs->ops->rename(fs, fs_src, fs_dst);
out:
  if (me != NULL) me->fs->refs--;
  mgos_vfs_unlock();
  LOG(LL_DEBUG, ("%s -> %s => %p %s -> %s => %d", src, dst, fs,
                 (fs_src ? fs_src : ""), (fs_dst ? fs_dst : ""), ret));
  free(fs_src);
  free(fs_dst);
  return ret;
}
#if MGOS_VFS_DEFINE_LIBC_API
int rename(const char *src, const char *dst) {
  return mgos_vfs_rename(src, dst);
}
#endif
#if MGOS_VFS_DEFINE_LIBC_REENT_API
int _rename_r(struct _reent *r, const char *src, const char *dst) {
  (void) r;
  return mgos_vfs_rename(src, dst);
}
#endif

#if MG_ENABLE_DIRECTORY_LISTING

struct mgos_vfs_DIR {
/*
 * On ESP32 we must embed the DIR at the top of the struct to pass through
 * the IDF's own VFS layer.
 */
#ifdef ESP_PLATFORM
  DIR x;
#endif
  struct mgos_vfs_mount_entry *me;
  DIR *fs_dir;
};

struct mgos_vfs_dirent {
  struct mgos_vfs_mount_entry *me;
  struct dirent *fs_dirent;
};

DIR *mgos_vfs_opendir(const char *path) {
  struct mgos_vfs_DIR *dir = NULL;
  char *fs_path = NULL;
  DIR *fs_dir = NULL;
  struct mgos_vfs_fs *fs = NULL;
  struct mgos_vfs_mount_entry *me = find_mount_by_path(path, &fs_path);
  mgos_vfs_lock();
  if (me == NULL) {
    errno = ENOENT;
    goto out;
  }
  fs = me->fs;
  dir = (struct mgos_vfs_DIR *) calloc(1, sizeof(*dir));
  if (dir == NULL) {
    goto out;
  }
  fs_dir = fs->ops->opendir(fs, fs_path);
  if (fs_dir != NULL) {
    dir->me = me;
    dir->fs_dir = fs_dir;
  } else {
    free(dir);
    dir = NULL;
  }
out:
  if (me != NULL && dir == NULL) me->fs->refs--;
  mgos_vfs_unlock();
  LOG(LL_DEBUG,
      ("%s => %p %s %p => %p (refs %d)", path, fs, (fs_path ? fs_path : ""),
       fs_dir, dir, (me ? me->fs->refs : -1)));
  free(fs_path);
  return (DIR *) dir;
}
#if MGOS_VFS_DEFINE_LIBC_DIR_API
DIR *opendir(const char *path) {
  return mgos_vfs_opendir(path);
}
#endif
#if MGOS_VFS_DEFINE_LIBC_REENT_DIR_API
DIR *_opendir_r(struct _reent *r, const char *path) {
  (void) r;
  return mgos_vfs_opendir(path);
}
#endif

struct dirent *mgos_vfs_readdir(DIR *pdir) {
  struct dirent *de;
  struct mgos_vfs_DIR *dir = (struct mgos_vfs_DIR *) pdir;
  if (dir == NULL) {
    errno = EBADF;
    de = NULL;
    goto out;
  }
  mgos_vfs_lock();
  de = dir->me->fs->ops->readdir(dir->me->fs, dir->fs_dir);
  mgos_vfs_unlock();
out:
  return de;
}
#if MGOS_VFS_DEFINE_LIBC_DIR_API
struct dirent *readdir(DIR *pdir) {
  return mgos_vfs_readdir(pdir);
}
#endif
#if MGOS_VFS_DEFINE_LIBC_REENT_DIR_API
struct dirent *_readdir_r(struct _reent *r, DIR *pdir) {
  (void) r;
  return mgos_vfs_readdir(pdir);
}
#endif

int mgos_vfs_closedir(DIR *pdir) {
  int ret;
  DIR *fs_dir = NULL;
  struct mgos_vfs_fs *fs = NULL;
  struct mgos_vfs_mount_entry *me = NULL;
  struct mgos_vfs_DIR *dir = (struct mgos_vfs_DIR *) pdir;
  if (dir == NULL) {
    errno = EBADF;
    ret = -1;
    goto out;
  }
  me = dir->me;
  fs = me->fs;
  fs_dir = dir->fs_dir;
  mgos_vfs_lock();
  ret = fs->ops->closedir(fs, fs_dir);
  dir->me->fs->refs--;
  mgos_vfs_unlock();
out:
  LOG(LL_DEBUG, ("%p => %p:%p => %d (refs %d)", dir, fs, fs_dir, ret,
                 (me ? me->fs->refs : -1)));
  free(dir);
  return ret;
}
#if MGOS_VFS_DEFINE_LIBC_DIR_API
int closedir(DIR *pdir) {
  return mgos_vfs_closedir(pdir);
}
#endif
#if MGOS_VFS_DEFINE_LIBC_REENT_DIR_API
int _closedir_r(struct _reent *r, DIR *pdir) {
  (void) r;
  return mgos_vfs_closedir(pdir);
}
#endif

#endif /* MG_ENABLE_DIRECTORY_LISTING */

#ifdef CS_MMAP
#define MMAP_DESCS_ADD_SIZE 4

static struct mbuf mgos_vfs_mmap_descs_mbuf;

struct mgos_vfs_mmap_desc *mgos_vfs_mmap_descs = NULL;

/*
 * The memory returned by alloc_mmap_desc is zeroed out. NULL can only be
 * returned if there's no memory to grow descriptors mbuf.
 */
static struct mgos_vfs_mmap_desc *alloc_mmap_desc(void) {
  size_t i;
  size_t descs_cnt = mgos_vfs_mmap_descs_cnt();
  for (i = 0; i < descs_cnt; i++) {
    if (mgos_vfs_mmap_descs[i].base == NULL) {
      return &mgos_vfs_mmap_descs[i];
    }
  }

  /* Failed to find an empty descriptor, need to grow mbuf */
  int old_len = mgos_vfs_mmap_descs_mbuf.len;
  if (mbuf_append(&mgos_vfs_mmap_descs_mbuf, NULL,
                  MMAP_DESCS_ADD_SIZE * sizeof(struct mgos_vfs_mmap_desc)) ==
      0) {
    /* Out of memory */
    return NULL;
  }
  mgos_vfs_mmap_descs =
      (struct mgos_vfs_mmap_desc *) mgos_vfs_mmap_descs_mbuf.buf;
  memset(mgos_vfs_mmap_descs_mbuf.buf + old_len, 0,
         mgos_vfs_mmap_descs_mbuf.len - old_len);

  /* Call this function again; this time it should succeed */
  return alloc_mmap_desc();
}

static void free_mmap_desc(struct mgos_vfs_mmap_desc *desc) {
  if (desc->fs != NULL) {
    desc->fs->ops->munmap(desc);
    desc->fs->refs--;
    desc->fs = NULL;
  }
  memset(desc, 0, sizeof(*desc));
}

void *mgos_vfs_mmap(void *addr, size_t len, int prot, int flags, int vfd,
                    off_t offset) {
  bool ok = true;

  if (len == 0) {
    return NULL;
  }

  mgos_vfs_lock();

  struct mgos_vfs_mmap_desc *desc = alloc_mmap_desc();
  if (desc == NULL) {
    LOG(LL_ERROR, ("cannot allocate mmap desc"));
    ok = false;
    goto clean;
  }

  struct mgos_vfs_mount_entry *me = find_mount_by_vfd(vfd);
  if (me == NULL) {
    LOG(LL_ERROR, ("can't find mount entry by vfd %d", vfd));
    ok = false;
    goto clean;
  }

  /*
   * fs->refs was incremented by find_mount_by_vfd, it'll be decremented back
   * in free_mmap_desc.
   */

  desc->fs = me->fs;

  if (desc->fs->ops->read_mmapped_byte == NULL) {
    LOG(LL_ERROR, ("filesystem doesn't support mmapping"));
    ok = false;
    goto clean;
  }

  if (desc->fs->ops->mmap(vfd, len, desc) == -1) {
    LOG(LL_ERROR, ("fs-specific mmap failure"));
    ok = false;
    goto clean;
  }

  desc->base = MMAP_BASE_FROM_DESC(desc);

  /*
   * Close the file descriptor. This breaks the posix-like mmap abstraction but
   * file descriptors are a scarse resource here.
   */
  int t = mgos_vfs_close(vfd);
  if (t != 0) {
    LOG(LL_ERROR, ("failed to close descr after mmapping: %d", t));
    return NULL;
  }

clean:
  mgos_vfs_unlock();

  if (!ok) {
    if (desc != NULL) {
      free_mmap_desc(desc);
      desc = NULL;
    }
    return MAP_FAILED;
  }

  (void) addr;
  (void) prot;
  (void) flags;
  (void) offset;

  return desc->base;
}

int mgos_vfs_munmap(void *addr, size_t len) {
  mgos_vfs_lock();

  int ret = -1;
  size_t descs_cnt = mgos_vfs_mmap_descs_cnt();

  if (addr != NULL) {
    size_t i;
    for (i = 0; i < descs_cnt; i++) {
      if (mgos_vfs_mmap_descs[i].base == addr) {
        free_mmap_desc(&mgos_vfs_mmap_descs[i]);
        ret = 0;
        goto clean;
      }
    }
  }

  /* didn't find the mapping with the given addr */
  LOG(LL_ERROR, ("Didn't find the mapping for the addr %p", addr));

clean:
  mgos_vfs_unlock();
  (void) len;
  return ret;
}

void mgos_vfs_mmap_init(void) {
  mbuf_init(&mgos_vfs_mmap_descs_mbuf, 0);
}

int mgos_vfs_mmap_descs_cnt(void) {
  return mgos_vfs_mmap_descs_mbuf.len / sizeof(struct mgos_vfs_mmap_desc);
}

struct mgos_vfs_mmap_desc *mgos_vfs_mmap_desc_get(int idx) {
  return &mgos_vfs_mmap_descs[idx];
}

#if MGOS_VFS_DEFINE_LIBC_MMAP_API
void *mmap(void *addr, size_t len, int prot, int flags, int vfd, off_t offset) {
  return mgos_vfs_mmap(addr, len, prot, flags, ARCH_FD_TO_VFS_FD(vfd), offset);
}

int munmap(void *addr, size_t len) {
  return mgos_vfs_munmap(addr, len);
}
#endif /* MGOS_VFS_DEFINE_LIBC_MMAP_API */
#endif /* CS_MMAP */

size_t mgos_get_fs_size(void) {
  size_t res;
  struct mgos_vfs_mount_entry *me = find_mount_by_path("/", NULL);
  if (me == NULL) return 0;
  mgos_vfs_lock();
  res = me->fs->ops->get_space_total(me->fs);
  me->fs->refs--;
  mgos_vfs_unlock();
  return res;
}

size_t mgos_get_free_fs_size(void) {
  size_t res;
  struct mgos_vfs_mount_entry *me = find_mount_by_path("/", NULL);
  if (me == NULL) return 0;
  mgos_vfs_lock();
  res = me->fs->ops->get_space_free(me->fs);
  me->fs->refs--;
  mgos_vfs_unlock();
  return res;
}

void mgos_fs_gc(void) {
  mgos_vfs_gc("/");
}

static bool mgos_vfs_umount_entry(struct mgos_vfs_mount_entry *me, bool force) {
  bool ret = false;
  if (me->fs->refs > 0 && !force) {
    errno = EBUSY;
    return false;
  }
  SLIST_REMOVE(&s_mounts, me, mgos_vfs_mount_entry, next);
  ret = me->fs->ops->umount(me->fs);
  if (ret) {
    if (me->fs->dev) {
      me->fs->dev->refs--;
      mgos_vfs_dev_close(me->fs->dev);
    }
    free(me->fs);
    free(me);
  }
  return ret;
}

bool mgos_vfs_umount(const char *path) {
  bool ret = false;
  struct mgos_vfs_fs *fs = NULL;
  struct mgos_vfs_mount_entry *me = find_mount_by_path(path, NULL);
  if (me == NULL) return false;
  mgos_vfs_lock();
  me->fs->refs--; /* Drop the ref taken by find */
  fs = me->fs;
  if (strcmp(me->prefix, path) == 0) { /* Must specify mount point exactly */
    ret = mgos_vfs_umount_entry(me, false);
  }
  mgos_vfs_unlock();
  LOG(LL_INFO, ("%s %p %d", path, fs, ret));
  (void) fs;
  return ret;
}

void mgos_vfs_umount_all(void) {
  LOG(LL_INFO, ("Unmounting filesystems"));
  struct mgos_vfs_mount_entry *me, *met;
  mgos_vfs_lock();
  SLIST_FOREACH_SAFE(me, &s_mounts, next, met) {
    mgos_vfs_umount_entry(me, true /* force */);
  }
  mgos_vfs_unlock();
}

bool mgos_vfs_gc(const char *path) {
  bool ret = false;
  struct mgos_vfs_fs *fs = NULL;
  struct mgos_vfs_mount_entry *me = find_mount_by_path(path, NULL);
  if (me == NULL) return false;
  mgos_vfs_lock();
  me->fs->refs--; /* Drop the ref taken by find */
  fs = me->fs;
  ret = fs->ops->gc(fs);
  mgos_vfs_unlock();
  LOG(LL_INFO, ("%s %p %d", path, fs, ret));
  return ret;
}
