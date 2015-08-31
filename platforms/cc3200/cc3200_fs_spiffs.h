#ifndef _CC3200_FS_SPIFFS_H_
#define _CC3200_FS_SPIFFS_H_

#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include "spiffs.h"

#include "cc3200_fs.h"

/* libc interface */
int fs_spiffs_open(const char *pathname, int flags, mode_t mode);
int fs_spiffs_close(int fd);
ssize_t fs_spiffs_read(int fd, void *buf, size_t count);
ssize_t fs_spiffs_write(int fd, const void *buf, size_t count);
int fs_spiffs_fstat(int fd, struct stat *s);
off_t fs_spiffs_lseek(int fd, off_t offset, int whence);
int fs_spiffs_unlink(const char *filename);
int fs_spiffs_rename(const char *from, const char *to);

#endif /* _CC3200_FS_SPIFFS_H_ */
