#ifndef _CC3200_FS_SPIFFS_CONTAINER_H_
#define _CC3200_FS_SPIFFS_CONTAINER_H_

#include "cc3200_fs.h"

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

extern struct mount_info s_fsm;
void fs_close_container(struct mount_info *m);

#endif /* _CC3200_FS_SPIFFS_CONTAINER_H_ */
