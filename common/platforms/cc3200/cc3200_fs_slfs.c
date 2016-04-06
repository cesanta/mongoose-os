/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/* Standard libc interface to TI SimpleLink FS. */

#if CS_PLATFORM == CS_P_CC3200 && defined(CC3200_FS_SLFS)

#include "cc3200_fs_slfs.h"

#include <errno.h>

#include <inc/hw_types.h>
#include <simplelink/include/simplelink.h>
#include <simplelink/include/fs.h>

#include "common/cs_dbg.h"

extern int set_errno(int e); /* From cc3200_fs.c */

/*
 * With SLFS, you have to pre-declare max file size. Yes. Really.
 * 64K should be enough for everyone. Right?
 */
#ifndef FS_SLFS_MAX_FILE_SIZE
#define FS_SLFS_MAX_FILE_SIZE (64 * 1024)
#endif

struct sl_fd_info {
  _i32 fh;
  _off_t pos;
  size_t size;
};

static struct sl_fd_info s_sl_fds[MAX_OPEN_SLFS_FILES];

static int sl_fs_to_errno(_i32 r) {
  DBG(("SL error: %d", (int) r));
  switch (r) {
    case SL_FS_OK:
      return 0;
    case SL_FS_FILE_NAME_EXIST:
      return EEXIST;
    case SL_FS_WRONG_FILE_NAME:
      return EINVAL;
    case SL_FS_ERR_NO_AVAILABLE_NV_INDEX:
    case SL_FS_ERR_NO_AVAILABLE_BLOCKS:
      return ENOSPC;
    case SL_FS_ERR_FAILED_TO_ALLOCATE_MEM:
      return ENOMEM;
    case SL_FS_ERR_FILE_NOT_EXISTS:
      return ENOENT;
    case SL_FS_ERR_NOT_SUPPORTED:
      return ENOTSUP;
  }
  return ENXIO;
}

int fs_slfs_open(const char *pathname, int flags, mode_t mode) {
  int fd;
  for (fd = 0; fd < MAX_OPEN_SLFS_FILES; fd++) {
    if (s_sl_fds[fd].fh <= 0) break;
  }
  if (fd >= MAX_OPEN_SLFS_FILES) return set_errno(ENOMEM);
  struct sl_fd_info *fi = &s_sl_fds[fd];

  _u32 am = 0;
  fi->size = -1;
  if (pathname[0] == '/') pathname++;
  int rw = (flags & 3);
  if (rw == O_RDONLY) {
    SlFsFileInfo_t sl_fi;
    _i32 r = sl_FsGetInfo((const _u8 *) pathname, 0, &sl_fi);
    if (r == SL_FS_OK) {
      fi->size = sl_fi.FileLen;
    }
    am = FS_MODE_OPEN_READ;
  } else {
    if (!(flags & O_TRUNC) || (flags & O_APPEND)) {
      // FailFS files cannot be opened for append and will be truncated
      // when opened for write.
      return set_errno(ENOTSUP);
    }
    if (flags & O_CREAT) {
      am = FS_MODE_OPEN_CREATE(FS_SLFS_MAX_FILE_SIZE, 0);
    } else {
      am = FS_MODE_OPEN_WRITE;
    }
  }
  _i32 r = sl_FsOpen((_u8 *) pathname, am, NULL, &fi->fh);
  DBG(("sl_FsOpen(%s, 0x%x) = %d, %d", pathname, (int) am, (int) r,
       (int) fi->fh));
  if (r == SL_FS_OK) {
    fi->pos = 0;
    r = fd;
  } else {
    fi->fh = -1;
    r = set_errno(sl_fs_to_errno(r));
  }
  return r;
}

int fs_slfs_close(int fd) {
  struct sl_fd_info *fi = &s_sl_fds[fd];
  if (fi->fh <= 0) return set_errno(EBADF);
  _i32 r = sl_FsClose(fi->fh, NULL, NULL, 0);
  DBG(("sl_FsClose(%d) = %d", (int) fi->fh, (int) r));
  s_sl_fds[fd].fh = -1;
  return set_errno(sl_fs_to_errno(r));
}

ssize_t fs_slfs_read(int fd, void *buf, size_t count) {
  struct sl_fd_info *fi = &s_sl_fds[fd];
  if (fi->fh <= 0) return set_errno(EBADF);
  /* Simulate EOF. sl_FsRead @ file_size return SL_FS_ERR_OFFSET_OUT_OF_RANGE.
   */
  if (fi->size >= 0 && fi->pos == fi->size) return 0;
  _i32 r = sl_FsRead(fi->fh, fi->pos, buf, count);
  DBG(("sl_FsRead(%d, %d, %d) = %d", (int) fi->fh, (int) fi->pos, (int) count,
       (int) r));
  if (r >= 0) {
    fi->pos += r;
    return r;
  }
  return set_errno(sl_fs_to_errno(r));
}

ssize_t fs_slfs_write(int fd, const void *buf, size_t count) {
  struct sl_fd_info *fi = &s_sl_fds[fd];
  if (fi->fh <= 0) return set_errno(EBADF);
  _i32 r = sl_FsWrite(fi->fh, fi->pos, (_u8 *) buf, count);
  DBG(("sl_FsWrite(%d, %d, %d) = %d", (int) fi->fh, (int) fi->pos, (int) count,
       (int) r));
  if (r >= 0) {
    fi->pos += r;
    return r;
  }
  return set_errno(sl_fs_to_errno(r));
}

int fs_slfs_stat(const char *pathname, struct stat *s) {
  SlFsFileInfo_t sl_fi;
  _i32 r = sl_FsGetInfo((const _u8 *) pathname, 0, &sl_fi);
  if (r == SL_FS_OK) {
    s->st_mode = S_IFREG | 0666;
    s->st_nlink = 1;
    s->st_size = sl_fi.FileLen;
    return 0;
  }
  return set_errno(sl_fs_to_errno(r));
}

int fs_slfs_fstat(int fd, struct stat *s) {
  struct sl_fd_info *fi = &s_sl_fds[fd];
  if (fi->fh <= 0) return set_errno(EBADF);
  s->st_mode = 0666;
  s->st_mode = S_IFREG | 0666;
  s->st_nlink = 1;
  s->st_size = fi->size;
  return 0;
}

off_t fs_slfs_lseek(int fd, off_t offset, int whence) {
  if (s_sl_fds[fd].fh <= 0) return set_errno(EBADF);
  switch (whence) {
    case SEEK_SET:
      s_sl_fds[fd].pos = offset;
      break;
    case SEEK_CUR:
      s_sl_fds[fd].pos += offset;
      break;
    case SEEK_END:
      return set_errno(ENOTSUP);
  }
  return 0;
}

int fs_slfs_unlink(const char *filename) {
  return set_errno(sl_fs_to_errno(sl_FsDel((const _u8 *) filename, 0)));
}

int fs_slfs_rename(const char *from, const char *to) {
  return set_errno(ENOTSUP);
}

#endif /* CS_PLATFORM == CS_P_CC3200 && defined(CC3200_FS_SLFS) */
