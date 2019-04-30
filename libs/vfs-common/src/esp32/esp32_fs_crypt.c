/*
 * Copyright (c) 2014-2018 Cesanta Software Limited
 * All rights reserved
 *
 * Licensed under the Apache License, Version 2.0 (the ""License"");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an ""AS IS"" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "esp32_fs.h"

#include "spiffs.h"
#include "spiffs_nucleus.h"

#include "esp_spi_flash.h"

#include "mbedtls/aes.h"

#include "common/cs_dbg.h"
#include "mgos_hal.h"

/*
 * ESP32 contains a hardware-accelerated flash encryption module. It uses a key
 * securely stored in eFuses and applies AES-256 to 32-byte blocks of data.
 * It is described in detail here:
 *   https://esp-idf.readthedocs.io/en/latest/security/flash-encryption.html
 *
 * Why not use it directly, then? Because we cannot always satisfy the alignment
 * and size requirements. Further, the module it only supports working directly
 * with flash, it's not possible to get encryption results from registers or
 * feed data to be decrypted from RAM.
 *
 * However, we would still like to make use of the securely stored key that is
 * used to encrypt other parts of flash (app, NVS, boot loader etc). Since we
 * cannot make use of the encryption operation, we will perform encryption
 * ourselves using mbedTLS AES primitive (which may well use the hardware AES
 * accelerator module - a general purpose one which the chip also has)
 * but we derive the key by decrypting some area of flash. We look for an empty
 * area on flash (32 bytes of 0xff in plain text) and derive the key by
 * performing decryption of that area.
 *
 * To summarize, the file-level SPIFFS data encryption works as follows:
 *   - AES-256 in CBC mode (but no chaining, only single block mode).
 *   - Key derived by using hardware flash decryption of 32 bytes of 0xff.
 *   - For each encryption, object id and file offset are used to set up an IV.
 */

mbedtls_aes_context s_aes_ctx_enc, s_aes_ctx_dec;

bool esp32_fs_crypt_init(void) {
  uint8_t tmp[32];
  uint32_t addr = 0;
  for (addr = 0; addr < spi_flash_get_chip_size(); addr += 32) {
    mgos_wdt_feed();
    if (spi_flash_read(addr, tmp, sizeof(tmp)) != ESP_OK) {
      LOG(LL_ERROR, ("SPI read error at 0x%x", addr));
      return false;
    }
    int j;
    for (j = 0; j < sizeof(tmp); j++) {
      if (tmp[j] != 0xff) break;
    }
    if (j < sizeof(tmp)) continue;
    /* Found a suitably empty location, now decrypt it. */
    if (spi_flash_read_encrypted(addr, tmp, sizeof(tmp)) != ESP_OK) {
      LOG(LL_ERROR, ("SPI encrypted read error at 0x%x", addr));
      return false;
    }
    /* Now in tmp we have 32 x 0xff processed with the flash encryption key. */
    mbedtls_aes_init(&s_aes_ctx_enc);
    mbedtls_aes_setkey_enc(&s_aes_ctx_enc, tmp, 256);
    mbedtls_aes_init(&s_aes_ctx_dec);
    mbedtls_aes_setkey_dec(&s_aes_ctx_dec, tmp, 256);
    LOG(LL_INFO, ("FS encryption key set up, seed @ 0x%x", addr));
    return true;
  }
  LOG(LL_ERROR, ("Could not a suitable seed area for FS encryption"));
  return false;
}

bool mgos_vfs_fs_spiffs_encrypt_block(spiffs_obj_id obj_id, uint32_t offset,
                                      void *data, uint32_t len) {
  if (len % 16 != 0) return false;
  uint8_t *p = (uint8_t *) data;
  while (len > 0) {
    uint32_t iv[4] = {0xdeadbeef, obj_id, 0x900df00d, offset};
    if (mbedtls_aes_crypt_cbc(&s_aes_ctx_enc, MBEDTLS_AES_ENCRYPT, 16,
                              (uint8_t *) iv, p, p) != 0) {
      return false;
    }
    p += 16;
    len -= 16;
    offset += 16;
  }
  return true;
}

bool mgos_vfs_fs_spiffs_decrypt_block(spiffs_obj_id obj_id, uint32_t offset,
                                      void *data, uint32_t len) {
  if (len % 16 != 0) return false;
  uint8_t *p = (uint8_t *) data;
  while (len > 0) {
    uint32_t iv[4] = {0xdeadbeef, obj_id, 0x900df00d, offset};
    if (mbedtls_aes_crypt_cbc(&s_aes_ctx_dec, MBEDTLS_AES_DECRYPT, 16,
                              (uint8_t *) iv, p, p) != 0) {
      return false;
    }
    p += 16;
    len -= 16;
    offset += 16;
  }
  return true;
}
