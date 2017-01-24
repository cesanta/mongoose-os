/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_COMMON_SPIFFS_SPIFFS_VFS_H_
#define CS_COMMON_SPIFFS_SPIFFS_VFS_H_

#ifndef CS_SPIFFS_ENABLE_VFS
#define CS_SPIFFS_ENABLE_VFS 0
#endif

#ifndef CS_HAVE_DIRENT_H
#define CS_HAVE_DIRENT_H 0
#endif

#if CS_SPIFFS_ENABLE_VFS

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <common/platform.h>

#include <spiffs.h>

#ifndef CS_SPIFFS_ENABLE_ENCRYPTION
#define CS_SPIFFS_ENABLE_ENCRYPTION 0
#endif

int spiffs_vfs_open(spiffs *fs, const char *path, int flags, int mode);
int spiffs_vfs_close(spiffs *fs, int fd);
ssize_t spiffs_vfs_read(spiffs *fs, int fd, void *dst, size_t size);
size_t spiffs_vfs_write(spiffs *fs, int fd, const void *data, size_t size);
int spiffs_vfs_stat(spiffs *fs, const char *path, struct stat *st);
int spiffs_vfs_fstat(spiffs *fs, int fd, struct stat *st);
off_t spiffs_vfs_lseek(spiffs *fs, int fd, off_t offset, int whence);
int spiffs_vfs_rename(spiffs *fs, const char *src, const char *dst);
int spiffs_vfs_unlink(spiffs *fs, const char *path);

#if MG_ENABLE_DIRECTORY_LISTING
#include <dirent.h>
DIR *spiffs_vfs_opendir(spiffs *fs, const char *name);
struct dirent *spiffs_vfs_readdir(spiffs *fs, DIR *dir);
int spiffs_vfs_closedir(spiffs *fs, DIR *dir);
#endif

int set_spiffs_errno(spiffs *fs, const char *op, int res);

#if CS_SPIFFS_ENABLE_ENCRYPTION
bool spiffs_vfs_enc_fs(spiffs *fs);

/*
 * Name encrypotion/decryption routines.
 * Source and destination can be the same, both must be at least
 * SPIFFS_OBJ_NAME_LEN bytes long. Outputs are guaranteed to be
 * NUL-terminated.
 */
bool spiffs_vfs_enc_name(const char *name, char *enc_name, size_t enc_name_size);
bool spiffs_vfs_dec_name(const char *enc_name, char *name, size_t name_size);

/* Functions that must be provided by the platform */
bool spiffs_vfs_encrypt_block(spiffs_obj_id obj_id, uint32_t offset, void *data, uint32_t len);
bool spiffs_vfs_decrypt_block(spiffs_obj_id obj_id, uint32_t offset, void *data, uint32_t len);
#endif

#endif /* CS_SPIFFS_ENABLE_VFS */

#endif /* CS_COMMON_SPIFFS_SPIFFS_VFS_H_ */
