/* LIBC interface to FS. */

#include "cc3200_fs_spiffs.h"
#include "cc3200_fs_spiffs_container.h"

#include <errno.h>
#include <fcntl.h>
#include "spiffs_nucleus.h"

static int set_errno(struct mount_info *m, int res) {
  if (res < 0) errno = SPIFFS_errno(&m->fs);
  return res;
}

int fs_spiffs_open(const char *pathname, int flags, mode_t mode) {
  struct mount_info *m = &s_fsm;
  spiffs_mode sm = 0;
  int rw = (flags & 3);
  if (!s_fsm.valid) return -1;
  if (rw == O_RDONLY || rw == O_RDWR) sm |= SPIFFS_RDONLY;
  if (rw == O_WRONLY || rw == O_RDWR) sm |= SPIFFS_WRONLY;
  if (flags & O_CREAT) sm |= SPIFFS_CREAT;
  if (flags & O_TRUNC) sm |= SPIFFS_TRUNC;
  if (flags & O_APPEND) sm |= SPIFFS_APPEND;
  /* Supported in newer versions of SPIFFS. */
  /* if (flags && O_EXCL) sm |= SPIFFS_EXCL; */
  /* if (flags && O_DIRECT) sm |= SPIFFS_DIRECT; */

  int res = SPIFFS_open(&m->fs, (char *) pathname, sm, 0);
  dprintf(("open(%s, %d) -> %d\n", pathname, flags, res));
  return set_errno(m, res);
}

int fs_spiffs_close(int fd) {
  struct mount_info *m = &s_fsm;
  spiffs_fd *sfd;
  dprintf(("close(%d)\n", fd));
  if (!m->valid) return set_errno(m, EBADF);
  if (spiffs_fd_get(&m->fs, fd, &sfd) == SPIFFS_OK &&
      (sfd->flags & SPIFFS_WRONLY)) {
    /* We are closing a file open for writing, close the backing store
     * to avoid the dreaded SL_FS_FILE_HAS_NOT_BEEN_CLOSE_CORRECTLY. */
    SPIFFS_close(&m->fs, fd);
    fs_close_container(m);
  } else {
    SPIFFS_close(&m->fs, fd);
  }
  return 0;
}

ssize_t fs_spiffs_read(int fd, void *buf, size_t count) {
  struct mount_info *m = &s_fsm;
  dprintf(("read(%d, %u)\n", fd, count));
  if (!m->valid) return set_errno(m, EBADF);
  return set_errno(m, SPIFFS_read(&m->fs, fd, buf, count));
}

ssize_t fs_spiffs_write(int fd, const void *buf, size_t count) {
  struct mount_info *m = &s_fsm;
  dprintf(("write(%d, %u)\n", fd, count));
  if (!m->valid) return set_errno(m, EBADF);
  return set_errno(m, SPIFFS_write(&m->fs, fd, (void *) buf, count));
}

int fs_spiffs_fstat(int fd, struct stat *s) {
  int res;
  spiffs_stat ss;
  struct mount_info *m = &s_fsm;
  memset(s, 0, sizeof(*s));
  dprintf(("fstat(%d)\n", fd));
  if (!m->valid) return set_errno(m, EBADF);
  res = SPIFFS_fstat(&m->fs, fd, &ss);
  if (res < 0) return set_errno(m, res);
  s->st_ino = ss.obj_id;
  s->st_mode = 0666;
  s->st_nlink = 1;
  s->st_size = ss.size;
  return 0;
}

off_t fs_spiffs_lseek(int fd, off_t offset, int whence) {
  struct mount_info *m = &s_fsm;
  dprintf(("lseek(%d, %u, %d)\n", fd, offset, whence));
  if (!m->valid) return set_errno(m, EBADF);
  return set_errno(m, SPIFFS_lseek(&m->fs, fd, offset, whence));
}

int fs_spiffs_rename(const char *from, const char *to) {
  struct mount_info *m = &s_fsm;
  dprintf(("rename(%s, %s)\n", from, to));
  if (!m->valid) return set_errno(m, EBADF);
  int res = SPIFFS_rename(&m->fs, (char *) from, (char *) to);
  if (res == 0) fs_close_container(m);
  return set_errno(m, res);
}

int fs_spiffs_unlink(const char *filename) {
  struct mount_info *m = &s_fsm;
  dprintf(("unlink(%s)\n", filename));
  if (!m->valid) return set_errno(m, EBADF);
  int res = SPIFFS_remove(&m->fs, (char *) filename);
  if (res == 0) fs_close_container(m);
  return set_errno(m, res);
}
