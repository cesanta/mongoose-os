/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_PLATFORMS_CC3200_CC3200_FS_FAILFS_H_
#define CS_SMARTJS_PLATFORMS_CC3200_CC3200_FS_FAILFS_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#include "cc3200_fs.h"

/* libc interface */
int fs_failfs_open(const char *pathname, int flags, mode_t mode);
int fs_failfs_close(int fd);
ssize_t fs_failfs_read(int fd, void *buf, size_t count);
ssize_t fs_failfs_write(int fd, const void *buf, size_t count);
int fs_failfs_stat(const char *pathname, struct stat *s);
int fs_failfs_fstat(int fd, struct stat *s);
off_t fs_failfs_lseek(int fd, off_t offset, int whence);
int fs_failfs_unlink(const char *filename);

#endif /* CS_SMARTJS_PLATFORMS_CC3200_CC3200_FS_FAILFS_H_ */
