/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "common/spiffs/spiffs_vfs.h"

bool spiffs_vfs_encrypt_block(spiffs_obj_id obj_id, uint32_t offset, void *data,
                              uint32_t len) {
  uint32_t tmp[8];
  if (len != 32) return false;
  memcpy(tmp, data, len);
  /* TODO(rojer): Implement actual encryption. */
  for (int i = 0; i < 8; i++) {
    tmp[i] ^= (0xdeadbeef ^ (obj_id << 16) ^ offset);
  }
  memcpy(data, tmp, len);
  (void) obj_id;
  (void) offset;
  return true;
}

bool spiffs_vfs_decrypt_block(spiffs_obj_id obj_id, uint32_t offset, void *data,
                              uint32_t len) {
  uint32_t tmp[8];
  if (len != 32) return false;
  memcpy(tmp, data, len);
  /* TODO(rojer): Implement actual decryption. */
  for (int i = 0; i < 8; i++) {
    tmp[i] ^= (0xdeadbeef ^ (obj_id << 16) ^ offset);
  }
  memcpy(data, tmp, len);
  (void) obj_id;
  (void) offset;
  return true;
}
