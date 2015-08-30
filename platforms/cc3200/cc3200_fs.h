#ifndef _CC3200_FS_H_
#define _CC3200_FS_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "spiffs.h"

struct v7;

typedef unsigned long long _u64;

struct mount_info {
  spiffs fs;
  _i32 fh;             /* FailFS file handle, or -1 if not open yet. */
  _u64 seq;            /* Sequence counter for the mounted container. */
  _u32 valid : 1;      /* 1 if this filesystem has been mounted. */
  _u32 cidx : 1;       /* Which of the two containers is currently mounted. */
  _u32 rw : 1;         /* 1 if the underlying fh is r/w. */
  _u32 formatting : 1; /* 1 if the filesystem is being formatted. */
  /* SPIFFS work area and file descriptor space. */
  _u8 *work;
  _u8 *fds;
  _u32 fds_size;
};

void fs_close_container(struct mount_info *m);

/* libc interface */
int fs_open(const char *pathname, int flags, mode_t mode);
int fs_close(int fd);
ssize_t fs_read(int fd, void *buf, size_t count);
ssize_t fs_write(int fd, const void *buf, size_t count);
int fs_fstat(int fd, struct stat *s);
off_t fs_lseek(int fd, off_t offset, int whence);

#ifdef CC3200_FS_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

int init_fs(struct v7 *v7);

#endif /* _CC3200_FS_H_ */
