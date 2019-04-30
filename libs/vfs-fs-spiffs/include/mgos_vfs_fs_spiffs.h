/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#ifndef CS_FW_SRC_MGOS_VFS_FS_SPIFFS_H_
#define CS_FW_SRC_MGOS_VFS_FS_SPIFFS_H_

#include <stdbool.h>

#include <spiffs.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef MGOS_SPIFFS_DEFAULT_MAX_OPEN_FILES
#define MGOS_SPIFFS_DEFAULT_MAX_OPEN_FILES 10
#endif

#ifndef MGOS_SPIFFS_DEFAULT_BLOCK_SIZE
#define MGOS_SPIFFS_DEFAULT_BLOCK_SIZE 4096
#endif

#ifndef MGOS_SPIFFS_DEFAULT_PAGE_SIZE
#define MGOS_SPIFFS_DEFAULT_PAGE_SIZE 256
#endif

#ifndef MGOS_SPIFFS_DEFAULT_ERASE_SIZE
#define MGOS_SPIFFS_DEFAULT_ERASE_SIZE 4096
#endif

#define MGOS_VFS_FS_TYPE_SPIFFS "SPIFFS"

#if CS_SPIFFS_ENABLE_ENCRYPTION
bool mgos_vfs_fs_spiffs_enc_fs(spiffs *fs);

/*
 * Name encrypotion/decryption routines.
 * Source and destination can be the same, both must be at least
 * SPIFFS_OBJ_NAME_LEN bytes long. Outputs are guaranteed to be
 * NUL-terminated.
 */
bool mgos_vfs_fs_spiffs_enc_name(const char *name, char *enc_name,
                                 size_t enc_name_size);
bool mgos_vfs_fs_spiffs_dec_name(const char *enc_name, char *name,
                                 size_t name_size);

/* Functions that must be provided by the platform */
bool mgos_vfs_fs_spiffs_encrypt_block(spiffs_obj_id obj_id, uint32_t offset,
                                      void *data, uint32_t len);
bool mgos_vfs_fs_spiffs_decrypt_block(spiffs_obj_id obj_id, uint32_t offset,
                                      void *data, uint32_t len);
#endif

#ifdef __cplusplus
}
#endif

#endif /* CS_FW_SRC_MGOS_VFS_FS_SPIFFS_H_ */
