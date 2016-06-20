/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_SPIFFS_H_
#define CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_SPIFFS_H_

#include <stdio.h>

#include <common/platform.h>

#include "spiffs.h"

#define MAX_OPEN_SPIFFS_FILES 8

/* libc interface */
int fs_spiffs_open(const char *pathname, int flags, mode_t mode);
int fs_spiffs_close(int fd);
ssize_t fs_spiffs_read(int fd, void *buf, size_t count);
ssize_t fs_spiffs_write(int fd, const void *buf, size_t count);
int fs_spiffs_stat(const char *pathname, struct stat *s);
int fs_spiffs_fstat(int fd, struct stat *s);
off_t fs_spiffs_lseek(int fd, off_t offset, int whence);
int fs_spiffs_unlink(const char *filename);
int fs_spiffs_rename(const char *from, const char *to);

DIR *fs_spiffs_opendir(const char *dir_name);
struct dirent *fs_spiffs_readdir(DIR *dir);
int fs_spiffs_closedir(DIR *dir);
int fs_spiffs_rmdir(const char *path);
int fs_spiffs_mkdir(const char *path, mode_t mode);

#endif /* CS_FW_PLATFORMS_CC3200_SRC_CC3200_FS_SPIFFS_H_ */
