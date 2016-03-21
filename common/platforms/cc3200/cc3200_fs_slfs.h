/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_SMARTJS_PLATFORMS_CC3200_CC3200_FS_SLFS_H_
#define CS_SMARTJS_PLATFORMS_CC3200_CC3200_FS_SLFS_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>

#define MAX_OPEN_SLFS_FILES 8

/* Indirect libc interface - same functions, different names. */
int fs_slfs_open(const char *pathname, int flags, mode_t mode);
int fs_slfs_close(int fd);
ssize_t fs_slfs_read(int fd, void *buf, size_t count);
ssize_t fs_slfs_write(int fd, const void *buf, size_t count);
int fs_slfs_stat(const char *pathname, struct stat *s);
int fs_slfs_fstat(int fd, struct stat *s);
off_t fs_slfs_lseek(int fd, off_t offset, int whence);
int fs_slfs_unlink(const char *filename);

#endif /* CS_SMARTJS_PLATFORMS_CC3200_CC3200_FS_SLFS_H_ */
