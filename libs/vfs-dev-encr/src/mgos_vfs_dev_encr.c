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

#include "mgos_vfs_dev_encr.h"

#include "common/cs_dbg.h"
#include "common/mg_str.h"

#include "frozen.h"

#include "mbedtls/aes.h"

#include "mgos_vfs_dev.h"

#define ENCR_MAX_KEY_LEN 32
#define ENCR_BLOCK_SIZE 16

#ifndef MGOS_VFS_DEV_ENCR_DEBUG_KEY
#define MGOS_VFS_DEV_ENCR_DEBUG_KEY 0
#endif

struct mgos_vfs_dev_encr_data {
  struct mgos_vfs_dev *io_dev;
  struct mgos_vfs_dev *key_dev;
  uint8_t *key;
  uint8_t key_len;
};

static enum mgos_vfs_dev_err encr_get_key(struct mgos_vfs_dev_encr_data *dd,
                                          void *key) {
  if (dd->key != NULL) {
    memcpy(key, dd->key, dd->key_len);
    return MGOS_VFS_DEV_ERR_NONE;
  }
  return mgos_vfs_dev_read(dd->key_dev, 0 /* offset */, dd->key_len, key);
}

static void __attribute__((noinline)) zeroize(void *p, size_t len) {
  memset(p, 0, len);
}

enum mgos_vfs_dev_err encr_dev_init(struct mgos_vfs_dev *dev,
                                    struct mgos_vfs_dev *io_dev,
                                    const uint8_t *key,
                                    struct mgos_vfs_dev *key_dev, int key_len,
                                    bool testing) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  struct mgos_vfs_dev_encr_data *dd =
      (struct mgos_vfs_dev_encr_data *) calloc(1, sizeof(*dd));
  uint8_t key1[ENCR_MAX_KEY_LEN], key2[ENCR_MAX_KEY_LEN];
  if (dd == NULL) {
    res = MGOS_VFS_DEV_ERR_NOMEM;
    goto out;
  }
  dd->io_dev = io_dev;
  dd->key_dev = key_dev;
  dd->key_len = key_len;
  if (key != NULL) {
    dd->key = (uint8_t *) malloc(key_len);
    if (dd->key == NULL) {
      res = MGOS_VFS_DEV_ERR_NOMEM;
      goto out;
    }
    memcpy(dd->key, key, key_len);
  }
  {
    if ((res = encr_get_key(dd, key1)) != 0) goto out;
    if ((res = encr_get_key(dd, key2)) != 0) goto out;
    if (memcmp(key1, key2, key_len) != 0) {
      LOG(LL_ERROR, ("Key device must yield the same key every time"));
      res = MGOS_VFS_DEV_ERR_INVAL;
      goto out;
    }
    /* Perform basic sanity check on the key. */
    {
      uint8_t i, n;
      for (i = 1, n = 0; i < key_len; i++) {
        if (key1[i] == key1[i - 1]) n++;
      }
      if (n >= key_len - 4) {
        LOG(LL_WARN, ("Encryption key is unset or trivial!"));
        if (!testing) {
          LOG(LL_ERROR, ("Bad key, set 'testing: true' to override."));
          res = MGOS_VFS_DEV_ERR_INVAL;
          goto out;
        }
      }
    }
  }
  dd->io_dev->refs++;
  if (dd->key_dev != NULL) dd->key_dev->refs++;
  dev->dev_data = dd;
  res = MGOS_VFS_DEV_ERR_NONE;
out:
  if (res != MGOS_VFS_DEV_ERR_NONE) free(dd);
  zeroize(key1, sizeof(key1));
  zeroize(key2, sizeof(key2));
  return res;
}

enum mgos_vfs_dev_err mgos_vfs_dev_encr_open(struct mgos_vfs_dev *dev,
                                             const char *opts) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  uint8_t *key = NULL;
  char *dev_name = NULL;
  struct mg_str algo = MG_NULL_STR;
  char *key_dev_name = NULL, *key_dev_type = NULL, *key_dev_opts = NULL;
  struct mgos_vfs_dev *io_dev = NULL, *key_dev = NULL;
  struct json_token algo_tok = JSON_INVALID_TOKEN;
  struct json_token kdo_json_tok = JSON_INVALID_TOKEN;
  int key_len = 0, key_str_len = 0, testing = true;
  json_scanf(opts, strlen(opts),
             ("{dev: %Q, algo: %T, key: %H, key_dev: %Q, "
              "key_dev_type: %Q, key_dev_opts: %T, testing: %B}"),
             &dev_name, &algo_tok, &key_str_len, &key, &key_dev_name,
             &key_dev_type, &kdo_json_tok, &testing);
  if (dev_name == NULL) {
    LOG(LL_ERROR, ("Device name is required"));
    goto out;
  }
  if (((key != NULL) + (key_dev_name != NULL) + (key_dev_type != NULL)) != 1) {
    LOG(LL_ERROR, ("One of key, key device name or type must be set"));
    goto out;
  }
  io_dev = mgos_vfs_dev_open(dev_name);
  if (io_dev == NULL) {
    LOG(LL_ERROR, ("Unable to open %s", dev_name));
    goto out;
  }
  if (key_dev_name != NULL) {
    key_dev = mgos_vfs_dev_open(key_dev_name);
    if (key_dev == NULL) {
      LOG(LL_ERROR, ("Unable to open key device %s", key_dev_name));
      goto out;
    }
  } else if (key_dev_type != NULL) {
    const char *kdo = "";
    if (kdo_json_tok.len > 0) {
      key_dev_opts = (char *) mg_strdup_nul(
                         mg_mk_str_n(kdo_json_tok.ptr, kdo_json_tok.len)).p;
      if (key_dev_opts == NULL) {
        res = MGOS_VFS_DEV_ERR_NOMEM;
        goto out;
      }
      kdo = key_dev_opts;
    }
    key_dev = mgos_vfs_dev_create(key_dev_type, kdo);
    if (key_dev == NULL) {
      LOG(LL_ERROR, ("Unable to create key device %s %s", key_dev_type, kdo));
      goto out;
    }
  }
  algo = mg_mk_str_n(algo_tok.ptr, algo_tok.len);
  if (algo.len == 0) algo = mg_mk_str("AES-128");
  if (mg_vcasecmp(&algo, "AES-128") == 0) {
    key_len = 16;
  } else if (mg_vcasecmp(&algo, "AES-192") == 0) {
    key_len = 24;
  } else if (mg_vcasecmp(&algo, "AES-256") == 0) {
    key_len = 32;
  } else {
    LOG(LL_ERROR, ("Unknown algo %.*s", (int) algo.len, algo.p));
    goto out;
  }
  if (key_str_len != 0 && key_len != key_str_len) {
    LOG(LL_ERROR, ("Length of the key provided does not match chosen algorithm "
                   "(%d vs %d)",
                   key_str_len, key_len));
    goto out;
  }

  LOG(LL_INFO, ("Algo %.*s, key dev %p", (int) algo.len, algo.p, key_dev));

  res = encr_dev_init(dev, io_dev, key, key_dev, key_len, testing);

out:
  mgos_vfs_dev_close(io_dev);
  mgos_vfs_dev_close(key_dev);
  if (key != NULL) {
    zeroize(key, key_str_len);
    free(key);
  }
  free(dev_name);
  free(key_dev_name);
  free(key_dev_type);
  free(key_dev_opts);
  return res;
}

static enum mgos_vfs_dev_err mgos_vfs_dev_encr_read(struct mgos_vfs_dev *dev,
                                                    size_t offset, size_t len,
                                                    void *dst) {
  size_t off = offset, l, kl_bits;
  bool aes_ctx_valid = false;
  mbedtls_aes_context aes_ctx;
  uint32_t key[ENCR_MAX_KEY_LEN / sizeof(uint32_t)];
  uint8_t *p;
  struct mgos_vfs_dev_encr_data *dd =
      (struct mgos_vfs_dev_encr_data *) dev->dev_data;
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  if (offset % ENCR_BLOCK_SIZE != 0) {
    LOG(LL_ERROR, ("Unaligned access: %lu @ %lu", (unsigned long) len,
                   (unsigned long) offset));
    goto out;
  }
  l = len & ~(ENCR_BLOCK_SIZE - 1);
  p = (uint8_t *) dst;
  if ((res = encr_get_key(dd, key)) != 0) goto out;
  mbedtls_aes_init(&aes_ctx);
  aes_ctx_valid = true;
  kl_bits = ((size_t) dd->key_len) * 8;
  if (l > 0) {
    if ((res = mgos_vfs_dev_read(dd->io_dev, off, l, dst)) != 0) {
      goto out;
    }
    while (l > 0) {
      key[0] ^= off;
      if (mbedtls_aes_setkey_dec(&aes_ctx, (void *) key, kl_bits) != 0) {
        res = MGOS_VFS_DEV_ERR_IO;
        goto out;
      }
      if (mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, p, p) != 0) {
        res = MGOS_VFS_DEV_ERR_IO;
        goto out;
      }
      key[0] ^= off;
      p += ENCR_BLOCK_SIZE;
      off += ENCR_BLOCK_SIZE;
      l -= ENCR_BLOCK_SIZE;
    }
  }
  l = len & (ENCR_BLOCK_SIZE - 1);
  if (l > 0) {
    uint8_t buf[ENCR_BLOCK_SIZE];
    if ((res = mgos_vfs_dev_read(dd->io_dev, off, ENCR_BLOCK_SIZE, buf)) != 0) {
      goto out;
    }
    key[0] ^= off;
    if (mbedtls_aes_setkey_dec(&aes_ctx, (void *) key, kl_bits) != 0) {
      res = MGOS_VFS_DEV_ERR_IO;
      goto out;
    }
    if (mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_DECRYPT, buf, buf) != 0) {
      res = MGOS_VFS_DEV_ERR_IO;
      goto out;
    }
    memcpy(p, buf, l);
  }
  res = MGOS_VFS_DEV_ERR_NONE;
out:
  if (aes_ctx_valid) mbedtls_aes_free(&aes_ctx);
  zeroize(&aes_ctx, sizeof(aes_ctx));
  zeroize(key, sizeof(key));
  return res;
}

static enum mgos_vfs_dev_err mgos_vfs_dev_encr_write(struct mgos_vfs_dev *dev,
                                                     size_t offset, size_t len,
                                                     const void *src) {
  size_t off, l, al, kl_bits;
  bool aes_ctx_valid = false;
  mbedtls_aes_context aes_ctx;
  const uint8_t *ps;
  uint8_t *tmp = NULL, *pd;
  uint32_t key[ENCR_MAX_KEY_LEN / sizeof(uint32_t)];
  struct mgos_vfs_dev_encr_data *dd =
      (struct mgos_vfs_dev_encr_data *) dev->dev_data;
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  if (offset % ENCR_BLOCK_SIZE != 0) {
    LOG(LL_ERROR, ("Unaligned access: %lu @ %lu", (unsigned long) len,
                   (unsigned long) offset));
    goto out;
  }
  if ((res = encr_get_key(dd, key)) != 0) goto out;
  mbedtls_aes_init(&aes_ctx);
  aes_ctx_valid = true;
  kl_bits = ((size_t) dd->key_len) * 8;
  al = (len + ENCR_BLOCK_SIZE - 1) & ~(ENCR_BLOCK_SIZE - 1);
  if ((tmp = malloc(len + ENCR_BLOCK_SIZE)) == NULL) {
    res = MGOS_VFS_DEV_ERR_NOMEM;
    goto out;
  }
  for (ps = src, pd = tmp, off = offset, l = 0; l < len;) {
    key[0] ^= off;
    if (mbedtls_aes_setkey_enc(&aes_ctx, (void *) key, kl_bits) != 0) {
      res = MGOS_VFS_DEV_ERR_IO;
      goto out;
    }
    if (len - l < ENCR_BLOCK_SIZE) {
      memset(pd, 0, ENCR_BLOCK_SIZE);
      memcpy(pd, ps, len - l);
      ps = pd;
    }
    if (mbedtls_aes_crypt_ecb(&aes_ctx, MBEDTLS_AES_ENCRYPT, ps, pd) != 0) {
      res = MGOS_VFS_DEV_ERR_IO;
      goto out;
    }
    key[0] ^= off;
    ps += ENCR_BLOCK_SIZE;
    pd += ENCR_BLOCK_SIZE;
    off += ENCR_BLOCK_SIZE;
    l += ENCR_BLOCK_SIZE;
  }
  res = mgos_vfs_dev_write(dd->io_dev, offset, al, tmp);
out:
  if (aes_ctx_valid) mbedtls_aes_free(&aes_ctx);
  zeroize(&aes_ctx, sizeof(aes_ctx));
  zeroize(key, sizeof(key));
  free(tmp);
  return res;
}

static enum mgos_vfs_dev_err mgos_vfs_dev_encr_erase(struct mgos_vfs_dev *dev,
                                                     size_t offset,
                                                     size_t len) {
  struct mgos_vfs_dev_encr_data *dd =
      (struct mgos_vfs_dev_encr_data *) dev->dev_data;
  return mgos_vfs_dev_erase(dd->io_dev, offset, len);
}

static size_t mgos_vfs_dev_encr_get_size(struct mgos_vfs_dev *dev) {
  struct mgos_vfs_dev_encr_data *dd =
      (struct mgos_vfs_dev_encr_data *) dev->dev_data;
  return mgos_vfs_dev_get_size(dd->io_dev);
}

static enum mgos_vfs_dev_err mgos_vfs_dev_encr_close(struct mgos_vfs_dev *dev) {
  struct mgos_vfs_dev_encr_data *dd =
      (struct mgos_vfs_dev_encr_data *) dev->dev_data;
  enum mgos_vfs_dev_err res =
      (mgos_vfs_dev_close(dd->io_dev) ? MGOS_VFS_DEV_ERR_NONE
                                      : MGOS_VFS_DEV_ERR_IO);
  mgos_vfs_dev_close(dd->key_dev);
  free(dd->key);
  free(dd);
  return res;
}

static enum mgos_vfs_dev_err mgos_vfs_dev_encr_get_erase_sizes(
    struct mgos_vfs_dev *dev, size_t sizes[MGOS_VFS_DEV_NUM_ERASE_SIZES]) {
  struct mgos_vfs_dev_encr_data *dd =
      (struct mgos_vfs_dev_encr_data *) dev->dev_data;
  return mgos_vfs_dev_get_erase_sizes(dd->io_dev, sizes);
}

const struct mgos_vfs_dev_ops mgos_vfs_dev_encr_ops = {
    .open = mgos_vfs_dev_encr_open,
    .read = mgos_vfs_dev_encr_read,
    .write = mgos_vfs_dev_encr_write,
    .erase = mgos_vfs_dev_encr_erase,
    .get_size = mgos_vfs_dev_encr_get_size,
    .close = mgos_vfs_dev_encr_close,
    .get_erase_sizes = mgos_vfs_dev_encr_get_erase_sizes,
};

bool mgos_vfs_dev_encr_init(void) {
  return mgos_vfs_dev_register_type(MGOS_VFS_DEV_TYPE_ENCR,
                                    &mgos_vfs_dev_encr_ops);
}
