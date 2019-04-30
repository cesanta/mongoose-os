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

#include "mgos_vfs_fs_lfs.h"

#include <errno.h>
#include <stdio.h>

#include "mgos.h"
#include "mgos_vfs.h"

#include "lfs.h"

#define MGOS_LFS_DEFAULT_IO_SIZE 64
#define MGOS_LFS_DEFAULT_BLOCK_SIZE 4096

static const struct mgos_vfs_fs_ops mgos_vfs_fs_lfs_ops;

struct mgos_lfs_fd_info {
  int fd;
  lfs_file_t f;
  SLIST_ENTRY(mgos_lfs_fd_info) next;
};

struct mgos_lfs_data {
  struct lfs_config cfg;
  struct mgos_vfs_fs *fs;
  lfs_t lfs;
  SLIST_HEAD(fds, mgos_lfs_fd_info) fds;
  /* Number of blocks used. Requires traversal of all blocks in the FS and so is
   * expensive to compute, so we cache it. Note that 0 iis not a valid value
   * since at least 2 blocks are used for superblock and its copy. */
  int num_blocks_used;
};

static int mgos_lfs_read(const struct lfs_config *c, lfs_block_t block,
                         lfs_off_t off, void *buffer, lfs_size_t size) {
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) c->context;
  enum mgos_vfs_dev_err res = mgos_vfs_dev_read(
      fsd->fs->dev, block * c->block_size + off, size, buffer);
  return (res == MGOS_VFS_DEV_ERR_CORRUPT ? LFS_ERR_CORRUPT : res);
}

static int mgos_lfs_prog(const struct lfs_config *c, lfs_block_t block,
                         lfs_off_t off, const void *buffer, lfs_size_t size) {
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) c->context;
  enum mgos_vfs_dev_err res = mgos_vfs_dev_write(
      fsd->fs->dev, block * c->block_size + off, size, buffer);
  fsd->num_blocks_used = 0;
  return (res == MGOS_VFS_DEV_ERR_CORRUPT ? LFS_ERR_CORRUPT : res);
}

static int mgos_lfs_erase(const struct lfs_config *c, lfs_block_t block) {
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) c->context;
  enum mgos_vfs_dev_err res =
      mgos_vfs_dev_erase(fsd->fs->dev, block * c->block_size, c->block_size);
  fsd->num_blocks_used = 0;
  return (res == MGOS_VFS_DEV_ERR_CORRUPT ? LFS_ERR_CORRUPT : res);
}

static int mgos_lfs_sync(const struct lfs_config *c) {
  (void) c;
  return LFS_ERR_OK;
}

static bool mgos_vfs_fs_lfs_parse_opts(struct mgos_vfs_fs *fs,
                                       struct lfs_config *cfg,
                                       const char *opts) {
  bool r = false;
  cfg->read = mgos_lfs_read;
  cfg->prog = mgos_lfs_prog;
  cfg->erase = mgos_lfs_erase;
  cfg->sync = mgos_lfs_sync;
  cfg->read_size = MGOS_LFS_DEFAULT_IO_SIZE;
  cfg->block_size = MGOS_LFS_DEFAULT_BLOCK_SIZE;
  cfg->lookahead = 1024;
  lfs_size_t size = 0;
  if (opts != NULL) {
    json_scanf(opts, strlen(opts), "{size: %u, bs: %u, is: %u}", &size,
               &cfg->block_size, &cfg->read_size);
  }
  cfg->prog_size = cfg->read_size;
  if (size == 0) {
    size = mgos_vfs_dev_get_size(fs->dev);
    if (size == 0) {
      LOG(LL_ERROR, ("size not specified"));
      goto out;
    }
  }
  if (size % cfg->block_size != 0) {
    LOG(LL_ERROR, ("size (%u) is not a miltiple of block size (%u)",
                   (unsigned int) size, (unsigned int) cfg->block_size));
    goto out;
  }
  cfg->block_count = size / cfg->block_size;
  r = true;
out:
  return r;
}

static bool mgos_vfs_fs_lfs_mount(struct mgos_vfs_fs *fs, const char *opts) {
  int mr;
  bool ret = false;
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) calloc(1, sizeof(*fsd));
  struct lfs_config *cfg;
  if (fsd == NULL) goto out;
  fsd->fs = fs;
  cfg = &fsd->cfg;
  if (!mgos_vfs_fs_lfs_parse_opts(fs, cfg, opts)) goto out;
  cfg->context = fsd;
  mr = lfs_mount(&fsd->lfs, cfg);
  ret = (mr == LFS_ERR_OK);
  LOG((ret ? LL_DEBUG : LL_ERROR),
      ("size %u rs %u ps %u bs %u => %d",
       (unsigned int) (cfg->block_count * cfg->block_size),
       (unsigned int) cfg->read_size, (unsigned int) cfg->prog_size,
       (unsigned int) cfg->block_size, mr));
out:
  if (ret) {
    fs->fs_data = fsd;
  } else {
    free(fsd);
  }
  return ret;
}

static bool mgos_vfs_fs_lfs_mkfs(struct mgos_vfs_fs *fs, const char *opts) {
  int mr;
  bool ret = false;
  struct mgos_lfs_data fsd = {.fs = fs};
  struct lfs_config *cfg = &fsd.cfg;
  cfg->context = &fsd;
  if (!mgos_vfs_fs_lfs_parse_opts(fs, cfg, opts)) goto out;
  mr = lfs_format(&fsd.lfs, cfg);
  ret = (mr == LFS_ERR_OK);
  LOG((ret ? LL_DEBUG : LL_ERROR),
      ("size %u rs %u ps %u bs %u => %d",
       (unsigned int) (cfg->block_count * cfg->block_size),
       (unsigned int) cfg->read_size, (unsigned int) cfg->prog_size,
       (unsigned int) cfg->block_size, mr));
out:
  return ret;
}

static bool mgos_vfs_fs_lfs_umount(struct mgos_vfs_fs *fs) {
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  lfs_unmount(&fsd->lfs);
  free(fsd);
  return true;
}

static int count_used_blocks(void *arg, lfs_block_t block) {
  size_t *res = (size_t *) arg;
  (*res)++;
  (void) block;
  return LFS_ERR_OK;
}

static size_t mgos_vfs_fs_lfs_get_blocks_used(struct mgos_lfs_data *fsd) {
  size_t res = fsd->num_blocks_used;
  if (res == 0) {
    if (lfs_traverse(&fsd->lfs, count_used_blocks, &res) == LFS_ERR_OK) {
      fsd->num_blocks_used = res;
    } else {
      res = 0;
    }
  }
  return res;
}

static size_t mgos_vfs_fs_lfs_get_space_total(struct mgos_vfs_fs *fs) {
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  return fsd->cfg.block_size * fsd->cfg.block_count;
}

static size_t mgos_vfs_fs_lfs_get_space_used(struct mgos_vfs_fs *fs) {
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  return mgos_vfs_fs_lfs_get_blocks_used(fsd) * fsd->cfg.block_size;
}

static size_t mgos_vfs_fs_lfs_get_space_free(struct mgos_vfs_fs *fs) {
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  size_t used = mgos_vfs_fs_lfs_get_blocks_used(fsd);
  if (used == 0) return 0; /* Error */
  used *= fsd->cfg.block_size;
  return (mgos_vfs_fs_lfs_get_space_total(fs) - used);
}

static int mgos_lfs_err_to_errno(int r) {
  switch (r) {
    case LFS_ERR_OK:
      return 0;
    case LFS_ERR_IO:
    case LFS_ERR_CORRUPT:
      return ENXIO;
    case LFS_ERR_NOENT:
      return ENOENT;
    case LFS_ERR_EXIST:
      return EEXIST;
    case LFS_ERR_NOTDIR:
      return ENOTDIR;
    case LFS_ERR_ISDIR:
      return EISDIR;
    case LFS_ERR_NOTEMPTY:
      return ENOTEMPTY;
    case LFS_ERR_BADF:
      return EBADF;
    case LFS_ERR_INVAL:
      return EINVAL;
    case LFS_ERR_NOSPC:
      return ENOSPC;
    case LFS_ERR_NOMEM:
      return ENOMEM;
  }
  return ENXIO;
}

static off_t mgos_lfs_set_errno(off_t res) {
  if (res >= 0) return res;
  int ret = (errno = mgos_lfs_err_to_errno(res));
  return (ret < 0 ? ret : -ret);
}

static struct mgos_lfs_fd_info *mgos_lfs_get_fdi(struct mgos_lfs_data *fsd,
                                                 int fd) {
  struct mgos_lfs_fd_info *fdi;
  SLIST_FOREACH(fdi, &fsd->fds, next) {
    if (fdi->fd == fd) return fdi;
  }
  return NULL;
}

static int mgos_vfs_fs_lfs_open(struct mgos_vfs_fs *fs, const char *path,
                                int flags, int mode) {
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  struct mgos_lfs_fd_info *fdi;
  int lfs_flags, r;
  (void) mode;
  switch (flags & 3) {
    case O_RDONLY:
      lfs_flags = LFS_O_RDONLY;
      break;
    case O_WRONLY:
      lfs_flags = LFS_O_WRONLY;
      break;
    case O_RDWR:
      lfs_flags = LFS_O_RDWR;
      break;
    default:
      r = LFS_ERR_INVAL;
      goto out;
  }
  if (flags & O_CREAT) lfs_flags |= LFS_O_CREAT;
#ifdef O_EXCL
  if (flags & O_EXCL) lfs_flags |= LFS_O_EXCL;
#endif
  if (flags & O_TRUNC) lfs_flags |= LFS_O_TRUNC;
  if (flags & O_APPEND) lfs_flags |= LFS_O_APPEND;

  fdi = (struct mgos_lfs_fd_info *) calloc(1, sizeof(*fdi));
  if (fdi == NULL) {
    r = LFS_ERR_NOMEM;
    goto out;
  }
  r = lfs_file_open(&fsd->lfs, &fdi->f, path, lfs_flags);
  if (r == LFS_ERR_OK) {
    fdi->fd = 0;
    while (mgos_lfs_get_fdi(fsd, fdi->fd) != NULL) {
      fdi->fd = MGOS_VFS_VFD_TO_FS_FD(fdi->fd + 1);
      if (fdi->fd == 0) {
        /* Ran out of descriptors! */
        lfs_file_close(&fsd->lfs, &fdi->f);
        r = LFS_ERR_NOMEM;
        goto out;
      }
    }
    SLIST_INSERT_HEAD(&fsd->fds, fdi, next);
    r = fdi->fd;
  } else {
    free(fdi);
  }
out:
  return mgos_lfs_set_errno(r);
}

static int mgos_vfs_fs_lfs_close(struct mgos_vfs_fs *fs, int fd) {
  int r = LFS_ERR_OK;
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  struct mgos_lfs_fd_info *fdi = mgos_lfs_get_fdi(fsd, fd);
  if (fdi == NULL) {
    r = LFS_ERR_BADF;
    goto out;
  }
  r = lfs_file_close(&fsd->lfs, &fdi->f);
  /* Even if error is returned, file is no longer valid. */
  SLIST_REMOVE(&fsd->fds, fdi, mgos_lfs_fd_info, next);
  free(fdi);
out:
  return mgos_lfs_set_errno(r);
}

static ssize_t mgos_vfs_fs_lfs_read(struct mgos_vfs_fs *fs, int fd, void *dstv,
                                    size_t size) {
  int r = LFS_ERR_OK;
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  struct mgos_lfs_fd_info *fdi = mgos_lfs_get_fdi(fsd, fd);
  if (fdi == NULL) {
    r = LFS_ERR_BADF;
    goto out;
  }
  r = lfs_file_read(&fsd->lfs, &fdi->f, dstv, size);
out:
  return mgos_lfs_set_errno(r);
}

ssize_t mgos_vfs_fs_lfs_write(struct mgos_vfs_fs *fs, int fd, const void *datav,
                              size_t size) {
  int r = LFS_ERR_OK;
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  struct mgos_lfs_fd_info *fdi = mgos_lfs_get_fdi(fsd, fd);
  if (fdi == NULL) {
    r = LFS_ERR_BADF;
    goto out;
  }
  r = lfs_file_write(&fsd->lfs, &fdi->f, datav, size);
out:
  return mgos_lfs_set_errno(r);
}

int mgos_vfs_fs_lfs_stat(struct mgos_vfs_fs *fs, const char *path,
                         struct stat *st) {
  struct lfs_info info;
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  int r = lfs_stat(&fsd->lfs, path, &info);
  if (r != LFS_ERR_OK) goto out;
  memset(st, 0, sizeof(*st));
  st->st_mode = 0777;
  switch (info.type) {
    case LFS_TYPE_DIR:
      st->st_mode |= S_IFDIR;
      break;
    case LFS_TYPE_REG:
      st->st_mode |= S_IFREG;
      break;
    default:
      r = LFS_ERR_CORRUPT;
      goto out;
  }
  st->st_size = info.size;
  st->st_nlink = 1;
out:
  return mgos_lfs_set_errno(r);
}

int mgos_vfs_fs_lfs_fstat(struct mgos_vfs_fs *fs, int fd, struct stat *st) {
  int r = LFS_ERR_OK;
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  struct mgos_lfs_fd_info *fdi = mgos_lfs_get_fdi(fsd, fd);
  if (fdi == NULL) {
    r = LFS_ERR_BADF;
    goto out;
  }
  memset(st, 0, sizeof(*st));
  st->st_mode = S_IFREG | 0777;
  st->st_size = lfs_file_size(&fsd->lfs, &fdi->f);
  st->st_nlink = 1;
out:
  return mgos_lfs_set_errno(r);
}

static off_t mgos_vfs_fs_lfs_lseek(struct mgos_vfs_fs *fs, int fd, off_t offset,
                                   int whence) {
  off_t r = LFS_ERR_OK, lwh;
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  struct mgos_lfs_fd_info *fdi = mgos_lfs_get_fdi(fsd, fd);
  if (fdi == NULL) {
    r = LFS_ERR_BADF;
    goto out;
  }
  switch (whence) {
    case SEEK_SET:
      lwh = LFS_SEEK_SET;
      break;
    case SEEK_CUR:
      lwh = LFS_SEEK_CUR;
      break;
    case SEEK_END:
      lwh = LFS_SEEK_END;
      break;
    default:
      r = LFS_ERR_INVAL;
      goto out;
  }
  r = lfs_file_seek(&fsd->lfs, &fdi->f, offset, lwh);
out:
  return mgos_lfs_set_errno(r);
}

static int mgos_vfs_fs_lfs_rename(struct mgos_vfs_fs *fs, const char *src,
                                  const char *dst) {
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  return mgos_lfs_set_errno(lfs_rename(&fsd->lfs, src, dst));
}

static int mgos_vfs_fs_lfs_unlink(struct mgos_vfs_fs *fs, const char *path) {
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  return mgos_lfs_set_errno(lfs_remove(&fsd->lfs, path));
}

#if MG_ENABLE_DIRECTORY_LISTING
struct mgos_lfs_dir {
  DIR dir;
  lfs_dir_t lfsd;
  struct dirent de;
};

static DIR *mgos_vfs_fs_lfs_opendir(struct mgos_vfs_fs *fs, const char *path) {
  int r = LFS_ERR_OK;
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  struct mgos_lfs_dir *d = NULL;

  if ((d = (struct mgos_lfs_dir *) calloc(1, sizeof(*d))) == NULL) {
    r = LFS_ERR_NOMEM;
    goto out;
  }

  r = lfs_dir_open(&fsd->lfs, &d->lfsd, path);

out:
  if (r != LFS_ERR_OK) {
    free(d);
    d = NULL;
  }
  return (DIR *) d;
}

static struct dirent *mgos_vfs_fs_lfs_readdir(struct mgos_vfs_fs *fs,
                                              DIR *dir) {
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  struct mgos_lfs_dir *d = (struct mgos_lfs_dir *) dir;
  struct lfs_info info;
  int r = lfs_dir_read(&fsd->lfs, &d->lfsd, &info);
  if (r <= 0) goto out;
  strncpy(d->de.d_name, info.name,
          MAX(sizeof(d->de.d_name), sizeof(info.name)) - 1);
out:
  if (r <= 0) {
    mgos_lfs_set_errno(r);
    return NULL;
  }
  return &d->de;
}

static int mgos_vfs_fs_lfs_closedir(struct mgos_vfs_fs *fs, DIR *dir) {
  struct mgos_lfs_data *fsd = (struct mgos_lfs_data *) fs->fs_data;
  struct mgos_lfs_dir *d = (struct mgos_lfs_dir *) dir;
  int r = lfs_dir_close(&fsd->lfs, &d->lfsd);
  free(d);
  return mgos_lfs_set_errno(r);
}
#endif /* MG_ENABLE_DIRECTORY_LISTING */

static bool mgos_vfs_fs_lfs_gc(struct mgos_vfs_fs *fs) {
  /* TODO(rojer): Scrub free blocks. */
  (void) fs;
  return true;
}

static const struct mgos_vfs_fs_ops mgos_vfs_fs_lfs_ops = {
    .mkfs = mgos_vfs_fs_lfs_mkfs,
    .mount = mgos_vfs_fs_lfs_mount,
    .umount = mgos_vfs_fs_lfs_umount,
    .get_space_total = mgos_vfs_fs_lfs_get_space_total,
    .get_space_used = mgos_vfs_fs_lfs_get_space_used,
    .get_space_free = mgos_vfs_fs_lfs_get_space_free,
    .gc = mgos_vfs_fs_lfs_gc,
    .open = mgos_vfs_fs_lfs_open,
    .close = mgos_vfs_fs_lfs_close,
    .read = mgos_vfs_fs_lfs_read,
    .write = mgos_vfs_fs_lfs_write,
    .stat = mgos_vfs_fs_lfs_stat,
    .fstat = mgos_vfs_fs_lfs_fstat,
    .lseek = mgos_vfs_fs_lfs_lseek,
    .unlink = mgos_vfs_fs_lfs_unlink,
    .rename = mgos_vfs_fs_lfs_rename,
#if MG_ENABLE_DIRECTORY_LISTING
    .opendir = mgos_vfs_fs_lfs_opendir,
    .readdir = mgos_vfs_fs_lfs_readdir,
    .closedir = mgos_vfs_fs_lfs_closedir,
#endif
};

bool mgos_vfs_fs_lfs_init(void) {
  return mgos_vfs_fs_register_type(MGOS_VFS_FS_TYPE_LFS, &mgos_vfs_fs_lfs_ops);
}
