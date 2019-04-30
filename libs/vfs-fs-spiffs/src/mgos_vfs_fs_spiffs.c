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

#include "mgos_vfs_fs_spiffs.h"

#include <errno.h>
#include <stdio.h>

#include <spiffs.h>
#include <spiffs_nucleus.h>

#include "common/cs_base64.h"
#include "common/cs_dbg.h"
#include "common/mbuf.h"

#include "frozen.h"

#include "mgos_hal.h"
#include "mgos_vfs.h"

#if CS_SPIFFS_ENABLE_ENCRYPTION
#include "esp_flash_encrypt.h"
#endif

static const struct mgos_vfs_fs_ops mgos_vfs_fs_spiffs_ops;

struct mgos_vfs_fs_spiffs_data {
  spiffs fs;
  u8_t *work;
  u8_t *fds;
  bool encrypt;
};

#ifdef CS_MMAP
#define SPIFFS_PAGE_HEADER_SIZE (sizeof(spiffs_page_header))
#define LOG_PAGE_SIZE 256
#define SPIFFS_PAGE_DATA_SIZE ((LOG_PAGE_SIZE) - (SPIFFS_PAGE_HEADER_SIZE))

/*
 * New mapping descriptor, used by mgos_vfs_fs_spiffs_mmap and
 * mgos_vfs_mmap_spiffs_dummy_read
 */
static struct mgos_vfs_mmap_desc *s_cur_mmap_desc;

struct mgos_vfs_mmap_spiffs_data {
  /* Number of spiffs pages in the mmapped area */
  uint32_t pages_cnt;

  /* Page addresses */
  uint32_t *page_addrs;
};

static int mgos_vfs_mmap_spiffs_dummy_read(spiffs *fs, u32_t addr, u32_t size,
                                           u8_t *dst) {
  (void) size;

  if (s_cur_mmap_desc != NULL && dst >= DUMMY_MMAP_BUFFER_START &&
      dst < DUMMY_MMAP_BUFFER_END) {
    struct mgos_vfs_mmap_spiffs_data *mydata =
        (struct mgos_vfs_mmap_spiffs_data *) s_cur_mmap_desc->fs_data;

    if ((addr - SPIFFS_PAGE_HEADER_SIZE) % SPIFFS_CFG_LOG_PAGE_SZ(fs) == 0) {
      addr &= MMAP_ADDR_MASK;
      mydata->page_addrs[mydata->pages_cnt++] = addr;
    }
    return 1;
  }

  return 0;
}
#endif

static s32_t mgos_spiffs_read(spiffs *spfs, u32_t addr, u32_t size, u8_t *dst) {
  struct mgos_vfs_fs *fs = (struct mgos_vfs_fs *) spfs->user_data;
#ifdef CS_MMAP
  if (mgos_vfs_mmap_spiffs_dummy_read(spfs, addr, size, dst)) {
    return SPIFFS_OK;
  }
#endif
  if (mgos_vfs_dev_read(fs->dev, addr, size, dst) != 0) {
    return SPIFFS_ERR_NOT_READABLE;
  }
  return SPIFFS_OK;
}

static s32_t mgos_spiffs_write(spiffs *spfs, u32_t addr, u32_t size,
                               u8_t *src) {
  struct mgos_vfs_fs *fs = (struct mgos_vfs_fs *) spfs->user_data;
  if (mgos_vfs_dev_write(fs->dev, addr, size, src) != 0) {
    return SPIFFS_ERR_NOT_WRITABLE;
  }
  return SPIFFS_OK;
}

static s32_t mgos_spiffs_erase(spiffs *spfs, u32_t addr, u32_t size) {
  struct mgos_vfs_fs *fs = (struct mgos_vfs_fs *) spfs->user_data;
  if (mgos_vfs_dev_erase(fs->dev, addr, size) != 0) {
    return SPIFFS_ERR_ERASE_FAIL;
  }
  return SPIFFS_OK;
}

static bool mgos_vfs_fs_spiffs_mount_common(struct mgos_vfs_fs *fs,
                                            const char *opts, bool *check) {
  s32_t r = -1;
  unsigned int num_fds;
  spiffs_config cfg;
  spiffs *spfs;
  struct mgos_vfs_fs_spiffs_data *fsd =
      (struct mgos_vfs_fs_spiffs_data *) calloc(1, sizeof(*fsd));
  if (fsd == NULL) goto out;
  cfg.phys_addr = 0;
  cfg.phys_size = mgos_vfs_dev_get_size(fs->dev);
  cfg.log_block_size = MGOS_SPIFFS_DEFAULT_BLOCK_SIZE;
  cfg.log_page_size = MGOS_SPIFFS_DEFAULT_PAGE_SIZE;
  cfg.phys_erase_block = MGOS_SPIFFS_DEFAULT_ERASE_SIZE;
  num_fds = MGOS_SPIFFS_DEFAULT_MAX_OPEN_FILES;
  if (opts != NULL) {
    json_scanf(opts, strlen(opts),
               "{addr: %u, size: %u, bs: %u, ps: %u, es: %u, nfd: %u, "
               "check: %B, encr: %B}",
               &cfg.phys_addr, &cfg.phys_size, &cfg.log_block_size,
               &cfg.log_page_size, &cfg.phys_erase_block, &num_fds, check,
               &fsd->encrypt);
  }
  fsd->fds = (u8_t *) calloc(num_fds, sizeof(spiffs_fd));
  fsd->work = (u8_t *) calloc(2, cfg.log_page_size);
  if (fsd->fds == NULL || fsd->work == NULL) goto out;
  cfg.hal_read_f = mgos_spiffs_read;
  cfg.hal_write_f = mgos_spiffs_write;
  cfg.hal_erase_f = mgos_spiffs_erase;
  fsd->fs.user_data = fs;
  spfs = &fsd->fs;
  r = SPIFFS_mount(spfs, &cfg, fsd->work, fsd->fds, num_fds * sizeof(spiffs_fd),
                   NULL, 0, NULL);
  LOG((r == SPIFFS_OK ? LL_DEBUG : LL_ERROR),
      ("addr 0x%x size %u bs %u ps %u es %u nfd %u encr %d => %d",
       (unsigned int) cfg.phys_addr, (unsigned int) cfg.phys_size,
       (unsigned int) cfg.log_block_size, (unsigned int) cfg.log_page_size,
       (unsigned int) cfg.phys_erase_block, (unsigned int) num_fds,
       fsd->encrypt, (int) r));
out:
  fs->fs_data = fsd;
  return (r == SPIFFS_OK);
}

static bool mgos_vfs_fs_spiffs_mkfs(struct mgos_vfs_fs *fs, const char *opts) {
  /* SPIFFS requires MKFS before formatting. We don't expect it to succeed. */
  bool unused_check;
  bool ret = mgos_vfs_fs_spiffs_mount_common(fs, opts, &unused_check);
  struct mgos_vfs_fs_spiffs_data *fsd =
      (struct mgos_vfs_fs_spiffs_data *) fs->fs_data;
  if (fsd == NULL) return false;
  spiffs *spfs = &fsd->fs;
  if (ret) {
    SPIFFS_unmount(spfs);
    LOG(LL_WARN, ("There is a valid FS already, reformatting"));
  }
  ret = (SPIFFS_format(spfs) == SPIFFS_OK);
  free(fsd->work);
  free(fsd->fds);
  free(fsd);
  fs->fs_data = NULL;
  return ret;
}

static bool mgos_vfs_fs_spiffs_mount(struct mgos_vfs_fs *fs, const char *opts) {
  /* Run check by default, see
   * https://github.com/pellepl/spiffs/issues/137#issuecomment-287192259 */
  bool check = true;
  bool ret = mgos_vfs_fs_spiffs_mount_common(fs, opts, &check);
  struct mgos_vfs_fs_spiffs_data *fsd =
      (struct mgos_vfs_fs_spiffs_data *) fs->fs_data;
  spiffs *spfs = NULL;
  if (ret) {
    spfs = &fsd->fs;
    mgos_wdt_feed();
    if (check && SPIFFS_check(spfs) != SPIFFS_OK) {
      LOG(LL_ERROR, ("Filesystem is corrupted, continuing anyway"));
    }
#if CS_SPIFFS_ENABLE_ENCRYPTION
    mgos_wdt_feed();
    if (fsd->encrypt && !mgos_vfs_fs_spiffs_enc_fs(spfs)) {
      ret = false;
    }
#endif
  }
  if (!ret) {
    LOG(LL_ERROR, ("SPIFFS mount failed"));
    if (spfs != NULL && SPIFFS_mounted(spfs)) SPIFFS_unmount(spfs);
    if (fsd != NULL) {
      free(fsd->work);
      free(fsd->fds);
      free(fsd);
      fs->fs_data = NULL;
    }
  }
  return ret;
}

static bool mgos_vfs_fs_spiffs_umount(struct mgos_vfs_fs *fs) {
  struct mgos_vfs_fs_spiffs_data *fsd =
      (struct mgos_vfs_fs_spiffs_data *) fs->fs_data;
  SPIFFS_unmount(&fsd->fs);
  free(fsd->work);
  free(fsd->fds);
  free(fsd);
  return true;
}

static void mgos_vfs_fs_spiffs_get_info(struct mgos_vfs_fs *fs, u32_t *total,
                                        u32_t *used) {
  struct mgos_vfs_fs_spiffs_data *fsd =
      (struct mgos_vfs_fs_spiffs_data *) fs->fs_data;
  *total = *used = 0;
  SPIFFS_info(&fsd->fs, total, used);
}

static size_t mgos_vfs_fs_spiffs_get_space_total(struct mgos_vfs_fs *fs) {
  u32_t total, used;
  mgos_vfs_fs_spiffs_get_info(fs, &total, &used);
  return total;
}

static size_t mgos_vfs_fs_spiffs_get_space_used(struct mgos_vfs_fs *fs) {
  u32_t total, used;
  mgos_vfs_fs_spiffs_get_info(fs, &total, &used);
  return used;
}

static size_t mgos_vfs_fs_spiffs_get_space_free(struct mgos_vfs_fs *fs) {
  u32_t total, used;
  mgos_vfs_fs_spiffs_get_info(fs, &total, &used);
  return total - used;
}

#if CS_SPIFFS_ENABLE_ENCRYPTION
/* Note 1: initial state of file meta is all-1s.
 * Note 2: resetting a bit can be done without relocating the file,
 *         hence some boolean flags have inverted logic. */
struct file_meta {
  uint32_t plain_size : 24;
  uint32_t encryption_not_started : 1;
  uint32_t not_o_append : 1;
  uint32_t reserved;
};
#define DEFAULT_PLAIN_SIZE 0xffffff

/*
 * Names are Base64 encoded and must be NUL terminated.
 * They are also encrypted in blocks, so must come in block-sized chunks.
 */
#define MAX_PLAIN_NAME_LEN                                                   \
  ((((SPIFFS_OBJ_NAME_LEN - 1) * 6 / 8) / CS_SPIFFS_ENCRYPTION_BLOCK_SIZE) * \
   CS_SPIFFS_ENCRYPTION_BLOCK_SIZE)

/* file_meta size must match SPIFFS_OBJ_META_LEN. */
typedef char sizeof_file_meta_is_wrong
    [(SPIFFS_OBJ_META_LEN == sizeof(struct file_meta)) ? 1 : -1];

#endif

static int spiffs_err_to_errno(int r) {
  switch (r) {
    case SPIFFS_OK:
      return 0;
    case SPIFFS_ERR_FULL:
      return ENOSPC;
    case SPIFFS_ERR_NOT_FOUND:
      return ENOENT;
    case SPIFFS_ERR_NOT_WRITABLE:
    case SPIFFS_ERR_NOT_READABLE:
      return EACCES;
  }
  return ENXIO;
}

static int set_spiffs_errno(spiffs *spfs, int res) {
  int e = SPIFFS_errno(spfs);
  if (res >= 0) return res;
  errno = spiffs_err_to_errno(e);
  return -1;
}

static int mgos_vfs_fs_spiffs_open(struct mgos_vfs_fs *fs, const char *path,
                                   int flags, int mode) {
  spiffs *spfs = &((struct mgos_vfs_fs_spiffs_data *) fs->fs_data)->fs;
  spiffs_mode sm = 0;
  int rw = (flags & 3);
  (void) mode;
  if (rw == O_RDONLY || rw == O_RDWR) sm |= SPIFFS_RDONLY;
  if (rw == O_WRONLY || rw == O_RDWR) sm |= SPIFFS_WRONLY;
  if (flags & O_CREAT) sm |= SPIFFS_CREAT;
  if (flags & O_TRUNC) sm |= SPIFFS_TRUNC;
#ifdef O_EXCL
  if (flags & O_EXCL) sm |= SPIFFS_EXCL;
#endif

#if CS_SPIFFS_ENABLE_ENCRYPTION
  if (spfs->encrypted) {
    char enc_path[SPIFFS_OBJ_NAME_LEN];
    if (!mgos_vfs_fs_spiffs_enc_name(path, enc_path, sizeof(enc_path))) {
      errno = ENXIO;
      return -1;
    }
    path = enc_path;

    sm |= SPIFFS_RDONLY; /* Encryption always needs to be able to read. */
    int fd = SPIFFS_open(spfs, path, sm, 0);
    if (fd < 0 && SPIFFS_errno(spfs) == SPIFFS_ERR_OUT_OF_FILE_DESCS) {
      LOG(LL_ERROR, ("Too many open files!"));
    }
    if (fd >= 0 && (rw & O_WRONLY)) {
      spiffs_stat s;
      s32_t r = SPIFFS_fstat(spfs, fd, &s);
      if (r < 0) {
        set_spiffs_errno(spfs, r);
        SPIFFS_close(spfs, fd);
        return -1;
      }
      struct file_meta *fm = (struct file_meta *) s.meta;
      if (fm->plain_size == DEFAULT_PLAIN_SIZE || (flags & O_TRUNC)) {
        /* Can only happen to new files. */
        if (s.size != 0) {
          LOG(LL_ERROR, ("Corrupted encrypted file %s (es %u ps %u)", s.name,
                         s.size, fm->plain_size));
          SPIFFS_close(spfs, fd);
          errno = ENXIO;
          return -1;
        }
        fm->plain_size = 0;
        fm->encryption_not_started = false;
      }
      /*
       * O_APPEND requires special handling during write and must not be set on
       * the underlying file. It does not really need to be persisted, but we
       * don't have a per-fd state that we can use for it and creating one for
       * just one flag seems like an overkill. Yes, this may result in
       * additional
       * churn, but should be ok in most cases.
       */
      fm->not_o_append = !(flags & O_APPEND);
      r = SPIFFS_fupdate_meta(spfs, fd, fm);
      if (r < 0) {
        set_spiffs_errno(spfs, r);
        SPIFFS_close(spfs, fd);
        return -1;
      }
    }
    return set_spiffs_errno(spfs, fd);
  } else
#endif
  {
    if (flags & O_APPEND) sm |= SPIFFS_APPEND;
    int fd = set_spiffs_errno(spfs, SPIFFS_open(spfs, path, sm, 0));
    if (fd < 0 && SPIFFS_errno(spfs) == SPIFFS_ERR_OUT_OF_FILE_DESCS) {
      LOG(LL_ERROR, ("Too many open files!"));
    }
    return fd;
  }
}

static int mgos_vfs_fs_spiffs_close(struct mgos_vfs_fs *fs, int fd) {
  spiffs *spfs = &((struct mgos_vfs_fs_spiffs_data *) fs->fs_data)->fs;
  return set_spiffs_errno(spfs, SPIFFS_close(spfs, fd));
}

static ssize_t mgos_vfs_fs_spiffs_read(struct mgos_vfs_fs *fs, int fd,
                                       void *dstv, size_t size) {
  spiffs *spfs = &((struct mgos_vfs_fs_spiffs_data *) fs->fs_data)->fs;
#if CS_SPIFFS_ENABLE_ENCRYPTION
  spiffs_stat s;
  s32_t r = SPIFFS_fstat(spfs, fd, &s);
  if (r < 0) return set_spiffs_errno(spfs, r);
  const struct file_meta *fm = (const struct file_meta *) s.meta;
  if (spfs->encrypted && !fm->encryption_not_started) {
    LOG(LL_VERBOSE_DEBUG, ("enc_read %s (%u) %d", s.name, s.obj_id, size));
    if (fm->plain_size == DEFAULT_PLAIN_SIZE ||
        (s.size % CS_SPIFFS_ENCRYPTION_BLOCK_SIZE != 0)) {
      goto out_enc_err;
    }
    s32_t plain_off = SPIFFS_tell(spfs, fd);
    if (plain_off < 0) return set_spiffs_errno(spfs, plain_off);
    s32_t block_off =
        (plain_off - (plain_off % CS_SPIFFS_ENCRYPTION_BLOCK_SIZE));
    if (block_off != plain_off) {
      r = SPIFFS_lseek(spfs, fd, block_off, SPIFFS_SEEK_SET);
      if (r != block_off) {
        if (r < 0) {
          return set_spiffs_errno(spfs, r);
        } else {
          goto out_enc_err;
        }
      }
    }
    uint8_t *dst = (uint8_t *) dstv;
    int num_read = 0;
    while (num_read < size) {
      uint8_t block[CS_SPIFFS_ENCRYPTION_BLOCK_SIZE];
      r = SPIFFS_read(spfs, fd, block, sizeof(block));
      if (r != sizeof(block)) {
        if (r == 0 || SPIFFS_errno(spfs) == SPIFFS_ERR_END_OF_OBJECT) {
          break;
        } else {
          if (r < 0) {
            return set_spiffs_errno(spfs, r);
          } else {
            LOG(LL_ERROR, ("Expected to read %d @ %d, got %d", sizeof(block),
                           block_off, r));
            goto out_enc_err;
          }
        }
      }
      if (!mgos_vfs_fs_spiffs_decrypt_block(s.obj_id, block_off, block,
                                            sizeof(block))) {
        LOG(LL_ERROR, ("Decryption failed"));
        goto out_enc_err;
      }
      int to_skip = (num_read == 0 ? (plain_off - block_off) : 0);
      int to_copy = (fm->plain_size - block_off);
      if (to_copy > sizeof(block)) to_copy = sizeof(block);
      to_copy -= to_skip;
      if (to_copy > (size - num_read)) to_copy = (size - num_read);
      memcpy(dst, block + to_skip, to_copy);
      LOG(LL_VERBOSE_DEBUG, ("enc_read po %d bo %d ts %d tc %d nr %d",
                             plain_off, block_off, to_skip, to_copy, num_read));
      block_off += sizeof(block);
      num_read += to_copy;
      dst += to_copy;
    }
    LOG(LL_VERBOSE_DEBUG, ("%d @ %d => %d", size, plain_off, num_read));
    SPIFFS_lseek(spfs, fd, plain_off + num_read, SPIFFS_SEEK_SET);
    return num_read;

  out_enc_err:
    LOG(LL_ERROR, ("Corrupted encrypted file %s (es %u ps %u)", s.name, s.size,
                   fm->plain_size));
    errno = ENXIO;
    return -1;
  } else
#endif /* CS_SPIFFS_ENABLE_ENCRYPTION */
  {
    int n = SPIFFS_read(spfs, fd, dstv, size);
    if (n < 0 && SPIFFS_errno(spfs) == SPIFFS_ERR_END_OF_OBJECT) {
      /* EOF */
      n = 0;
    }
    return set_spiffs_errno(spfs, n);
  }
}

static ssize_t mgos_vfs_fs_spiffs_write(struct mgos_vfs_fs *fs, int fd,
                                        const void *datav, size_t size) {
  spiffs *spfs = &((struct mgos_vfs_fs_spiffs_data *) fs->fs_data)->fs;
#if CS_SPIFFS_ENABLE_ENCRYPTION
  spiffs_stat s;
  s32_t r = SPIFFS_fstat(spfs, fd, &s);
  if (r < 0) return set_spiffs_errno(spfs, r);
  struct file_meta *fm = (struct file_meta *) s.meta;
  if (spfs->encrypted && !fm->encryption_not_started) {
    if (fm->plain_size == DEFAULT_PLAIN_SIZE ||
        (s.size % CS_SPIFFS_ENCRYPTION_BLOCK_SIZE != 0)) {
      goto out_enc_err;
    }
    s32_t plain_off = SPIFFS_tell(spfs, fd);
    if (plain_off < 0) return set_spiffs_errno(spfs, plain_off);
    if (!(fm->not_o_append)) plain_off = fm->plain_size;
    s32_t block_off =
        (plain_off - (plain_off % CS_SPIFFS_ENCRYPTION_BLOCK_SIZE));
    uint8_t block[CS_SPIFFS_ENCRYPTION_BLOCK_SIZE];
    int prefix_len = plain_off - block_off;
    if (prefix_len > 0) {
      r = SPIFFS_lseek(spfs, fd, block_off, SPIFFS_SEEK_SET);
      if (r != block_off) {
        if (r < 0) {
          return set_spiffs_errno(spfs, r);
        } else {
          goto out_enc_err;
        }
      }
      r = SPIFFS_read(spfs, fd, block, prefix_len);
      if (r != prefix_len) {
        if (r < 0) {
          return set_spiffs_errno(spfs, r);
        } else {
          LOG(LL_ERROR,
              ("Expected to read %d @ %d, got %d", prefix_len, block_off, r));
          goto out_enc_err;
        }
      }
      if (!mgos_vfs_fs_spiffs_decrypt_block(s.obj_id, block_off, block,
                                            sizeof(block))) {
        LOG(LL_ERROR, ("Decryption failed"));
        goto out_enc_err;
      }
      r = SPIFFS_lseek(spfs, fd, block_off, SPIFFS_SEEK_SET);
      if (r != block_off) {
        if (r < 0) {
          return set_spiffs_errno(spfs, r);
        } else {
          goto out_enc_err;
        }
      }
    }
    uint8_t *data = (uint8_t *) datav;
    int num_written = 0;
    int to_skip = prefix_len;
    while (num_written < size) {
      int to_copy = (sizeof(block) - to_skip);
      if (to_copy > (size - num_written)) to_copy = size - num_written;
      memcpy(block + to_skip, data, to_copy);
      for (int i = to_skip + to_copy; i < sizeof(block); i++) {
        block[i] = 0;
      }
      if (!mgos_vfs_fs_spiffs_encrypt_block(s.obj_id, block_off, block,
                                            sizeof(block))) {
        LOG(LL_ERROR, ("Encryption failed"));
        if (num_written > 0) break;
        goto out_enc_err;
      }
      r = SPIFFS_write(spfs, fd, block, sizeof(block));
      if (r != sizeof(block)) {
        if (num_written > 0) break;
        if (r < 0) {
          return set_spiffs_errno(spfs, r);
        } else {
          LOG(LL_ERROR, ("Expected to write %d @ %d, got %d", sizeof(block),
                         block_off, r));
          goto out_enc_err;
        }
      }
      LOG(LL_DEBUG, ("enc_write s %d po %d bo %d ts %d tc %d nw %d", size,
                     plain_off, block_off, to_skip, to_copy, num_written));
      block_off += sizeof(block);
      num_written += to_copy;
      data += to_copy;
      to_skip = 0;
    }
    /* We have written some data, calls below should not cause failure. */
    int new_plain_off = plain_off + num_written;
    if (new_plain_off > fm->plain_size) {
      fm->plain_size = new_plain_off;
      r = SPIFFS_fupdate_meta(spfs, fd, fm);
      if (r < 0) {
        LOG(LL_ERROR, ("enc_write_update_meta: %d", r));
      }
    }
    LOG(LL_DEBUG, ("%d @ %d => %d, po %d ps %d", size, plain_off, num_written,
                   new_plain_off, (int) fm->plain_size));
    SPIFFS_lseek(spfs, fd, new_plain_off, SPIFFS_SEEK_SET);
    return num_written;

  out_enc_err:
    LOG(LL_ERROR, ("Corrupted encrypted file %s (es %u ps %u)", s.name, s.size,
                   fm->plain_size));
    errno = ENXIO;
    return -1;
  } else
#endif /* CS_SPIFFS_ENABLE_ENCRYPTION */
  {
    return set_spiffs_errno(spfs, SPIFFS_write(spfs, fd, (void *) datav, size));
  }
}

static void mgos_vfs_fs_spiffs_xlate_stat(spiffs *spfs, spiffs_stat *ss,
                                          struct stat *st) {
  st->st_ino = ss->obj_id;
  st->st_mode = S_IFREG | 0666;
  st->st_nlink = 1;
  st->st_size = ss->size;
#if CS_SPIFFS_ENABLE_ENCRYPTION
  const struct file_meta *fm = (const struct file_meta *) ss->meta;
  if (spfs->encrypted) {
    st->st_size = fm->plain_size;
  }
#else
  (void) spfs;
#endif
}

static int mgos_vfs_fs_spiffs_stat(struct mgos_vfs_fs *fs, const char *path,
                                   struct stat *st) {
  int res;
  spiffs_stat ss;
  spiffs *spfs = &((struct mgos_vfs_fs_spiffs_data *) fs->fs_data)->fs;
  memset(st, 0, sizeof(*st));
  /* Simulate statting the root directory. */
  if (path[0] == '\0' || strcmp(path, ".") == 0) {
    st->st_ino = 0;
    st->st_mode = S_IFDIR | 0777;
    st->st_nlink = 1;
    st->st_size = 0;
    return set_spiffs_errno(spfs, SPIFFS_OK);
  }
#if CS_SPIFFS_ENABLE_ENCRYPTION
  char enc_path[SPIFFS_OBJ_NAME_LEN];
  if (spfs->encrypted) {
    if (!mgos_vfs_fs_spiffs_enc_name(path, enc_path, sizeof(enc_path))) {
      errno = ENXIO;
      return -1;
    }
    path = enc_path;
  }
#endif
  res = SPIFFS_stat(spfs, path, &ss);
  if (res == SPIFFS_OK) {
    mgos_vfs_fs_spiffs_xlate_stat(spfs, &ss, st);
  }
  return set_spiffs_errno(spfs, res);
}

int mgos_vfs_fs_spiffs_fstat(struct mgos_vfs_fs *fs, int fd, struct stat *st) {
  int res;
  spiffs *spfs = &((struct mgos_vfs_fs_spiffs_data *) fs->fs_data)->fs;
  spiffs_stat ss;
  memset(st, 0, sizeof(*st));
  res = SPIFFS_fstat(spfs, fd, &ss);
  if (res == SPIFFS_OK) {
    mgos_vfs_fs_spiffs_xlate_stat(spfs, &ss, st);
  }
  return set_spiffs_errno(spfs, res);
}

static off_t mgos_vfs_fs_spiffs_lseek(struct mgos_vfs_fs *fs, int fd,
                                      off_t offset, int whence) {
  spiffs *spfs = &((struct mgos_vfs_fs_spiffs_data *) fs->fs_data)->fs;
#if CS_SPIFFS_ENABLE_ENCRYPTION
  spiffs_stat s;
  s32_t r = SPIFFS_fstat(spfs, fd, &s);
  if (r < 0) return set_spiffs_errno(spfs, r);
  const struct file_meta *fm = (const struct file_meta *) s.meta;
  if (fm->plain_size != DEFAULT_PLAIN_SIZE) {
    off_t cur = SPIFFS_tell(spfs, fd);
    switch (whence) {
      case SEEK_SET:
        if (offset >= fm->plain_size) {
          offset = fm->plain_size;
        }
        break;
      case SEEK_CUR:
        if (cur + offset > fm->plain_size) {
          whence = SEEK_SET;
          offset = fm->plain_size;
        }
        break;
      case SEEK_END:
        whence = SEEK_SET;
        offset = ((off_t) fm->plain_size) + offset;
        break;
    }
  }
#endif /* CS_SPIFFS_ENABLE_ENCRYPTION */
  return set_spiffs_errno(spfs, SPIFFS_lseek(spfs, fd, offset, whence));
}

static int mgos_vfs_fs_spiffs_rename(struct mgos_vfs_fs *fs, const char *src,
                                     const char *dst) {
  int res;
  spiffs *spfs = &((struct mgos_vfs_fs_spiffs_data *) fs->fs_data)->fs;
#if CS_SPIFFS_ENABLE_ENCRYPTION
  char enc_src[SPIFFS_OBJ_NAME_LEN], enc_dst[SPIFFS_OBJ_NAME_LEN];
  if (spfs->encrypted) {
    if (!mgos_vfs_fs_spiffs_enc_name(src, enc_src, sizeof(enc_src)) ||
        !mgos_vfs_fs_spiffs_enc_name(dst, enc_dst, sizeof(enc_dst))) {
      errno = ENXIO;
      return -1;
    }
    src = enc_src;
    dst = enc_dst;
  }
#endif
  /* Renaming file to itself should be a no-op. */
  if (strcmp(src, dst) == 0) return 0;
  {
    /*
     * POSIX rename requires that in case "to" exists, it be atomically replaced
     * with "from". The atomic part we can't do, but at least we can do replace.
     */
    spiffs_stat ss;
    res = SPIFFS_stat(spfs, dst, &ss);
    if (res == 0) {
      SPIFFS_remove(spfs, dst);
    }
  }
  return set_spiffs_errno(spfs, SPIFFS_rename(spfs, src, dst));
}

static int mgos_vfs_fs_spiffs_unlink(struct mgos_vfs_fs *fs, const char *path) {
  spiffs *spfs = &((struct mgos_vfs_fs_spiffs_data *) fs->fs_data)->fs;
#if CS_SPIFFS_ENABLE_ENCRYPTION
  char enc_path[SPIFFS_OBJ_NAME_LEN];
  if (spfs->encrypted) {
    if (!mgos_vfs_fs_spiffs_enc_name(path, enc_path, sizeof(enc_path))) {
      errno = ENXIO;
      return -1;
    }
    path = enc_path;
  }
#endif
  return set_spiffs_errno(spfs, SPIFFS_remove(spfs, path));
}

#if MG_ENABLE_DIRECTORY_LISTING
struct spiffs_dir {
  DIR dir;
  spiffs_DIR sdh;
  struct spiffs_dirent sde;
  struct dirent de;
};

static DIR *mgos_vfs_fs_spiffs_opendir(struct mgos_vfs_fs *fs,
                                       const char *path) {
  struct spiffs_dir *sd = NULL;
  spiffs *spfs = &((struct mgos_vfs_fs_spiffs_data *) fs->fs_data)->fs;

  if (path == NULL) {
    errno = EINVAL;
    return NULL;
  }

  if ((sd = (struct spiffs_dir *) calloc(1, sizeof(*sd))) == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  if (SPIFFS_opendir(spfs, path, &sd->sdh) == NULL) {
    free(sd);
    sd = NULL;
    errno = EINVAL;
  }

  return (DIR *) sd;
}

static struct dirent *mgos_vfs_fs_spiffs_readdir(struct mgos_vfs_fs *fs,
                                                 DIR *dir) {
  spiffs *spfs = &((struct mgos_vfs_fs_spiffs_data *) fs->fs_data)->fs;
  struct spiffs_dir *sd = (struct spiffs_dir *) dir;
  if (SPIFFS_readdir(&sd->sdh, &sd->sde) == SPIFFS_OK) {
    errno = EBADF;
    return NULL;
  }
  sd->de.d_ino = sd->sde.obj_id;
#if CS_SPIFFS_ENABLE_ENCRYPTION
  if (spfs->encrypted) {
    if (!mgos_vfs_fs_spiffs_dec_name((const char *) sd->sde.name, sd->de.d_name,
                                     sizeof(sd->de.d_name))) {
      LOG(LL_ERROR, ("Name decryption failed (%s)", sd->sde.name));
      errno = ENXIO;
      return NULL;
    }
  } else
#endif
  {
    memcpy(sd->de.d_name, sd->sde.name, SPIFFS_OBJ_NAME_LEN);
  }
  (void) spfs;
  return &sd->de;
}

static int mgos_vfs_fs_spiffs_closedir(struct mgos_vfs_fs *fs, DIR *dir) {
  struct spiffs_dir *sd = (struct spiffs_dir *) dir;
  if (dir != NULL) {
    SPIFFS_closedir(&sd->sdh);
    free(dir);
  }
  (void) fs;
  return 0;
}
#endif /* MG_ENABLE_DIRECTORY_LISTING */

static bool mgos_vfs_fs_spiffs_gc_all(spiffs *spfs) {
  u32_t total, used;
  if (SPIFFS_info(spfs, &total, &used) != SPIFFS_OK) return false;
  /* https://github.com/pellepl/spiffs/issues/135 */
  uint32_t del_before, del_after;
  do {
    del_before = spfs->stats_p_deleted;
    int r = SPIFFS_gc(spfs, total - used - 2 * spfs->cfg.log_page_size);
    del_after = spfs->stats_p_deleted;
    LOG(LL_DEBUG, ("GC result %d del pages %u -> %u", r,
                   (unsigned int) del_before, (unsigned int) del_after));
    (void) r;
    mgos_wdt_feed();
  } while (del_after < del_before);
  return true;
}

static bool mgos_vfs_fs_spiffs_gc(struct mgos_vfs_fs *fs) {
  struct mgos_vfs_fs_spiffs_data *fsd =
      (struct mgos_vfs_fs_spiffs_data *) fs->fs_data;
  return mgos_vfs_fs_spiffs_gc_all(&fsd->fs);
}

#if CS_SPIFFS_ENABLE_ENCRYPTION
bool mgos_vfs_fs_spiffs_enc_name(const char *name, char *enc_name,
                                 size_t enc_name_size) {
  uint8_t tmp[SPIFFS_OBJ_NAME_LEN];
  char tmp2[SPIFFS_OBJ_NAME_LEN];
  int name_len = strlen(name);
  int enc_name_len = 0;
  if (name_len > MAX_PLAIN_NAME_LEN || enc_name_size < SPIFFS_OBJ_NAME_LEN) {
    LOG(LL_ERROR, ("%s: name too long", name));
    return false;
  }
  memcpy(tmp, name, name_len);
  memset(tmp + name_len, 0, sizeof(tmp) - name_len);
  while (enc_name_len < name_len) {
    if (!mgos_vfs_fs_spiffs_encrypt_block(0, enc_name_len, tmp + enc_name_len,
                                          CS_SPIFFS_ENCRYPTION_BLOCK_SIZE)) {
      return false;
    }
    enc_name_len += CS_SPIFFS_ENCRYPTION_BLOCK_SIZE;
  }
  cs_base64_encode(tmp, enc_name_len, tmp2); /* NUL-terminates output. */
  LOG(LL_DEBUG, ("%s -> %s", name, tmp2));
  strncpy(enc_name, tmp2, enc_name_size);
  return true;
}

bool mgos_vfs_fs_spiffs_dec_name(const char *enc_name, char *name,
                                 size_t name_size) {
  int i;
  char tmp[SPIFFS_OBJ_NAME_LEN];
  int enc_name_len = 0;
  cs_base64_decode((const unsigned char *) enc_name, strlen(enc_name), tmp,
                   &enc_name_len);
  if (enc_name_len == 0 ||
      (enc_name_len % CS_SPIFFS_ENCRYPTION_BLOCK_SIZE != 0)) {
    return false;
  }
  for (i = 0; i < enc_name_len; i += CS_SPIFFS_ENCRYPTION_BLOCK_SIZE) {
    if (!mgos_vfs_fs_spiffs_decrypt_block(0, i, tmp + i,
                                          CS_SPIFFS_ENCRYPTION_BLOCK_SIZE)) {
      LOG(LL_ERROR, ("Decryption failed"));
      return false;
    }
    if (name_size - i < CS_SPIFFS_ENCRYPTION_BLOCK_SIZE) return false;
    memcpy(name + i, tmp + i, CS_SPIFFS_ENCRYPTION_BLOCK_SIZE);
  }
  while (i < name_size) {
    name[i++] = '\0';
  }
  LOG(LL_DEBUG, ("%s -> %s", enc_name, name));
  return true;
}

bool mgos_vfs_fs_spiffs_enc_fs(spiffs *spfs) {
  bool res = false;
  spiffs_DIR d;
  uint8_t *buf = NULL;
  struct spiffs_dirent e;
  spiffs_file fd = -1;
  size_t buf_size =
      ((SPIFFS_PAGES_PER_BLOCK(spfs) / 2 - 2) * SPIFFS_DATA_PAGE_SIZE(spfs));
  buf_size &= ~(CS_SPIFFS_ENCRYPTION_BLOCK_SIZE - 1);
  if ((buf = malloc(buf_size)) == NULL) goto out;
  if (SPIFFS_opendir(spfs, "/", &d) == NULL) {
    return false;
  }
  while (SPIFFS_readdir(&d, &e) != NULL) {
    char enc_name[SPIFFS_OBJ_NAME_LEN];
    struct file_meta *fm = (struct file_meta *) e.meta;
    LOG(LL_DEBUG,
        ("%s (%u) es %u ps %u", e.name, e.obj_id, e.size, fm->plain_size));
    if (fm->plain_size != DEFAULT_PLAIN_SIZE) continue; /* Already encrypted */
    if (!fm->encryption_not_started) {
      LOG(LL_ERROR, ("%s is partially encrypted; FS is corrupted.", e.name));
      goto out;
    }
    if (!mgos_vfs_fs_spiffs_enc_name((const char *) e.name, enc_name,
                                     sizeof(enc_name))) {
      LOG(LL_ERROR, ("%s: name encryption failed", e.name));
      goto out;
    }
    LOG(LL_INFO, ("Encrypting %s (id %u, size %d) -> %s", e.name, e.obj_id,
                  (int) e.size, enc_name));
    fd = SPIFFS_open_by_dirent(spfs, &e, SPIFFS_RDWR, 0);
    if (fd < 0) {
      LOG(LL_ERROR, ("%s: open failed: %d", e.name, SPIFFS_errno(spfs)));
      goto out;
    }
    fm->encryption_not_started = false;
    if (SPIFFS_fupdate_meta(spfs, fd, fm) != SPIFFS_OK) {
      LOG(LL_ERROR, ("%s: update_meta failed: %d", e.name, SPIFFS_errno(spfs)));
      goto out;
    }
    size_t enc_size = 0;
    fm->plain_size = 0;
    while (true) {
      int n = SPIFFS_read(spfs, fd, buf, buf_size);
      if (n < 0) {
        int err = SPIFFS_errno(spfs);
        if (err == SPIFFS_ERR_END_OF_OBJECT) break;
        LOG(LL_ERROR, ("%s: read failed: %d", e.name, err));
        goto out;
      } else if (n == 0) {
        break;
      }
      size_t blen = (size_t) n;
      /* Pad to block size. */
      while (blen % CS_SPIFFS_ENCRYPTION_BLOCK_SIZE != 0) {
        buf[blen++] = 0;
      }
      if (!mgos_vfs_fs_spiffs_encrypt_block(e.obj_id, fm->plain_size, buf,
                                            blen)) {
        LOG(LL_ERROR, ("%s: encrypt failed", e.name));
        goto out;
      }
      if (SPIFFS_lseek(spfs, fd, fm->plain_size, SPIFFS_SEEK_SET) !=
          fm->plain_size) {
        LOG(LL_ERROR, ("%s: seek failed: %d", e.name, SPIFFS_errno(spfs)));
        goto out;
      }
      int r = SPIFFS_write(spfs, fd, buf, blen);
      if (r != (int) blen) {
        LOG(LL_ERROR, ("%s: write %d @ %d failed: %d", e.name, (int) blen,
                       (int) fm->plain_size, SPIFFS_errno(spfs)));
        goto out;
      }
      fm->plain_size += n;
      enc_size += blen;
      mgos_wdt_feed();
    }
    if (SPIFFS_fupdate_meta(spfs, fd, fm) != SPIFFS_OK) {
      LOG(LL_ERROR,
          ("%s: final update_meta failed: %d", e.name, SPIFFS_errno(spfs)));
      goto out;
    }
    SPIFFS_close(spfs, fd);
    fd = -1;
    if (SPIFFS_rename(spfs, (const char *) e.name, enc_name) != SPIFFS_OK) {
      LOG(LL_ERROR, ("%s: rename failed: %d", e.name, SPIFFS_errno(spfs)));
      goto out;
    }
    /*
     * We have to start over, readdir can get confused if FS is mutated
     * while iterating.
     */
    SPIFFS_closedir(&d);
    if (SPIFFS_opendir(spfs, "/", &d) == NULL) {
      return false;
    }
  }
  /* Erase all plaintext left in the garbage. */
  mgos_vfs_fs_spiffs_gc_all(spfs);
  spfs->encrypted = true;
  res = true;
out:
  if (fd >= 0) SPIFFS_close(spfs, fd);
  SPIFFS_closedir(&d);
  free(buf);
  return res;
}
#endif /* CS_SPIFFS_ENABLE_ENCRYPTION */

#ifdef CS_MMAP
static void free_mmap_spiffs_data(struct mgos_vfs_mmap_spiffs_data *mydata) {
  if (mydata->page_addrs != NULL) {
    free(mydata->page_addrs);
    mydata->page_addrs = NULL;
  }
}

static int mgos_vfs_fs_spiffs_mmap(int vfd, size_t len,
                                   struct mgos_vfs_mmap_desc *desc) {
  int ret = 0;
  int pages_cnt = (len + SPIFFS_PAGE_DATA_SIZE - 1) / SPIFFS_PAGE_DATA_SIZE;

  struct mgos_vfs_mmap_spiffs_data *mydata = NULL;
  spiffs *spfs = &((struct mgos_vfs_fs_spiffs_data *) desc->fs->fs_data)->fs;

  /* Allocate SPIFFS-specific data for the mapping */
  mydata = (struct mgos_vfs_mmap_spiffs_data *) calloc(sizeof(*mydata), 1);
  if (mydata == NULL) {
    ret = -1;
    goto clean;
  }

  desc->fs_data = mydata;

  /*
   * Initialize SPIFFS-specific data; pages_cnt will be incremented gradually
   * by mgos_vfs_mmap_spiffs_dummy_read
   */
  mydata->pages_cnt = 0;
  mydata->page_addrs = (uint32_t *) calloc(sizeof(uint32_t), pages_cnt);
  if (mydata->page_addrs == NULL) {
    ret = -1;
    goto clean;
  }

  /*
   * Set s_cur_mmap_desc and invoke dummy read, so that we collect all page
   * addresses involved in the new mapping
   *
   * NOTE that the lock is already acquired by mgos_vfs_mmap()
   */
  s_cur_mmap_desc = desc;

  int32_t t = SPIFFS_read(spfs, MGOS_VFS_VFD_TO_FS_FD(vfd),
                          DUMMY_MMAP_BUFFER_START, len);
  if (t != (int32_t) len) {
    LOG(LL_ERROR,
        ("mmap dummy read failed: expected len: %d, actual: %d", len, t));
    ret = -1;
    goto clean;
  }

clean:
  s_cur_mmap_desc = NULL;
  if (ret == -1) {
    if (mydata != NULL) {
      free_mmap_spiffs_data(mydata);
      free(mydata);
      mydata = NULL;
    }
  }

  return ret;
}

static void mgos_vfs_fs_spiffs_munmap(struct mgos_vfs_mmap_desc *desc) {
  if (desc->fs_data != NULL) {
    free_mmap_spiffs_data((struct mgos_vfs_mmap_spiffs_data *) desc->fs_data);
    free(desc->fs_data);
    desc->fs_data = NULL;
  }
}

uint8_t mgos_vfs_fs_spiffs_read_mmapped_byte(struct mgos_vfs_mmap_desc *desc,
                                             uint32_t addr) {
  struct mgos_vfs_mmap_spiffs_data *mydata =
      (struct mgos_vfs_mmap_spiffs_data *) desc->fs_data;
  int page_num = addr / SPIFFS_PAGE_DATA_SIZE;
  int offset = addr % SPIFFS_PAGE_DATA_SIZE;
  size_t dev_offset = mydata->page_addrs[page_num] + offset;
  uint8_t ret;

  struct mgos_vfs_dev *dev = desc->fs->dev;
  mgos_vfs_dev_read(dev, dev_offset, 1, &ret);
  return ret;
}

/*
 * Relocate mmapped pages.
 *
 * TODO(dfrank): refactor it to use linked list,
 * see https://cesanta.slack.com/archives/C02NAP6QS/p1500387721632874
 */
void mgos_vfs_mmap_spiffs_on_page_move_hook(spiffs *fs, spiffs_file fh,
                                            spiffs_page_ix src_pix,
                                            spiffs_page_ix dst_pix) {
  size_t i, j;
  (void) fh;
  size_t descs_cnt = mgos_vfs_mmap_descs_cnt();
  for (i = 0; i < descs_cnt; i++) {
    struct mgos_vfs_mmap_desc *desc = mgos_vfs_mmap_desc_get(i);

    /*
     * Check if descriptor is in use and its filesystem is SPIFFS
     */
    if (desc->fs != NULL && desc->fs->ops == &mgos_vfs_fs_spiffs_ops) {
      /* Make sure it's the same SPIFFS instance */
      struct mgos_vfs_fs_spiffs_data *fsd =
          (struct mgos_vfs_fs_spiffs_data *) desc->fs->fs_data;
      if (&fsd->fs == fs) {
        struct mgos_vfs_mmap_spiffs_data *mydata =
            (struct mgos_vfs_mmap_spiffs_data *) desc->fs_data;
        for (j = 0; j < mydata->pages_cnt; j++) {
          uint32_t addr = mydata->page_addrs[j];
          uint32_t page_num = SPIFFS_PADDR_TO_PAGE(fs, addr);
          if (page_num == src_pix) {
            /*
             * Found mmapped page which was just moved, so adjust mmapped page
             * address.
             */
            int delta = (int) dst_pix - (int) src_pix;
            mydata->page_addrs[j] += delta * LOG_PAGE_SIZE;
          }
        }
      }
    }
  }
}

#endif /* CS_MMAP */

static const struct mgos_vfs_fs_ops mgos_vfs_fs_spiffs_ops = {
    .mkfs = mgos_vfs_fs_spiffs_mkfs,
    .mount = mgos_vfs_fs_spiffs_mount,
    .umount = mgos_vfs_fs_spiffs_umount,
    .get_space_total = mgos_vfs_fs_spiffs_get_space_total,
    .get_space_used = mgos_vfs_fs_spiffs_get_space_used,
    .get_space_free = mgos_vfs_fs_spiffs_get_space_free,
    .gc = mgos_vfs_fs_spiffs_gc,
    .open = mgos_vfs_fs_spiffs_open,
    .close = mgos_vfs_fs_spiffs_close,
    .read = mgos_vfs_fs_spiffs_read,
    .write = mgos_vfs_fs_spiffs_write,
    .stat = mgos_vfs_fs_spiffs_stat,
    .fstat = mgos_vfs_fs_spiffs_fstat,
    .lseek = mgos_vfs_fs_spiffs_lseek,
    .unlink = mgos_vfs_fs_spiffs_unlink,
    .rename = mgos_vfs_fs_spiffs_rename,
#if MG_ENABLE_DIRECTORY_LISTING
    .opendir = mgos_vfs_fs_spiffs_opendir,
    .readdir = mgos_vfs_fs_spiffs_readdir,
    .closedir = mgos_vfs_fs_spiffs_closedir,
#endif
#ifdef CS_MMAP
    .mmap = mgos_vfs_fs_spiffs_mmap,
    .munmap = mgos_vfs_fs_spiffs_munmap,
    .read_mmapped_byte = mgos_vfs_fs_spiffs_read_mmapped_byte,
#endif
};

bool mgos_vfs_fs_spiffs_init(void) {
  return mgos_vfs_fs_register_type(MGOS_VFS_FS_TYPE_SPIFFS,
                                   &mgos_vfs_fs_spiffs_ops);
}
