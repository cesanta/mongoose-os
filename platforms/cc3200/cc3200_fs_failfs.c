/* LIBC interface to TI FailFS. */

#include "cc3200_fs_failfs.h"
#include "config.h"

#include <errno.h>
#include <fcntl.h>

#include "hw_types.h"
#include "simplelink.h"
#include "fs.h"

struct ti_fd_info {
  _i32 fh;
  _off_t pos;
  size_t size;
};

static struct ti_fd_info s_ti_fds[MAX_OPEN_FAILFS_FILES];

static int sl_fs_to_errno(_i32 r) {
  dprintf(("SL error: %d\n", (int) r));
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

int fs_failfs_open(const char *pathname, int flags, mode_t mode) {
  int fd;
  for (fd = 0; fd < MAX_OPEN_FAILFS_FILES; fd++) {
    if (s_ti_fds[fd].fh <= 0) break;
  }
  if (fd >= MAX_OPEN_FAILFS_FILES) return set_errno(ENOMEM);
  struct ti_fd_info *fi = &s_ti_fds[fd];

  _u32 am = 0;
  fi->size = -1;
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
      am = FS_MODE_OPEN_CREATE(8192, 0);
    } else {
      am = FS_MODE_OPEN_WRITE;
    }
  }
  _i32 r = sl_FsOpen((_u8 *) pathname, am, NULL, &fi->fh);
  dprintf(("sl_FsOpen(%s, 0x%x) = %d, %d\n", pathname, (int) am, (int) r,
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

int fs_failfs_close(int fd) {
  struct ti_fd_info *fi = &s_ti_fds[fd];
  if (fi->fh <= 0) return set_errno(EBADF);
  _i32 r = sl_FsClose(fi->fh, NULL, NULL, 0);
  dprintf(("sl_FsClose(%d) = %d\n", (int) fi->fh, (int) r));
  s_ti_fds[fd].fh = -1;
  return set_errno(sl_fs_to_errno(r));
}

ssize_t fs_failfs_read(int fd, void *buf, size_t count) {
  struct ti_fd_info *fi = &s_ti_fds[fd];
  if (fi->fh <= 0) return set_errno(EBADF);
  /* Simulate EOF. sl_FsRead @ file_size return SL_FS_ERR_OFFSET_OUT_OF_RANGE.
   */
  if (fi->size >= 0 && fi->pos == fi->size) return 0;
  _i32 r = sl_FsRead(fi->fh, fi->pos, buf, count);
  dprintf(("sl_FsRead(%d, %d, %d) = %d\n", (int) fi->fh, (int) fi->pos,
           (int) count, (int) r));
  if (r >= 0) {
    fi->pos += r;
    return r;
  }
  return set_errno(sl_fs_to_errno(r));
}

ssize_t fs_failfs_write(int fd, const void *buf, size_t count) {
  struct ti_fd_info *fi = &s_ti_fds[fd];
  if (fi->fh <= 0) return set_errno(EBADF);
  _i32 r = sl_FsWrite(fi->fh, fi->pos, (_u8 *) buf, count);
  dprintf(("sl_FsWrite(%d, %d, %d) = %d\n", (int) fi->fh, (int) fi->pos,
           (int) count, (int) r));
  if (r >= 0) {
    fi->pos += r;
    return r;
  }
  return set_errno(sl_fs_to_errno(r));
}

int fs_failfs_fstat(int fd, struct stat *s) {
  struct ti_fd_info *fi = &s_ti_fds[fd];
  if (fi->fh <= 0) return set_errno(EBADF);
  memset(s, 0, sizeof(*s));
  s->st_ino = 0;
  s->st_mode = 0666;
  s->st_nlink = 1;
  s->st_size = fi->size;
  return 0;
}

off_t fs_failfs_lseek(int fd, off_t offset, int whence) {
  if (s_ti_fds[fd].fh <= 0) return set_errno(EBADF);
  switch (whence) {
    case SEEK_SET:
      s_ti_fds[fd].pos = offset;
      break;
    case SEEK_CUR:
      s_ti_fds[fd].pos += offset;
      break;
    case SEEK_END:
      return set_errno(ENOTSUP);
  }
  return 0;
}

int fs_failfs_unlink(const char *filename) {
  return set_errno(sl_fs_to_errno(sl_FsDel((const _u8 *) filename, 0)));
}
