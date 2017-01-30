/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

/* LIBC interface to SPIFFS. */

#include <spiffs_vfs.h>

#if CS_SPIFFS_ENABLE_VFS

#include <errno.h>
#include <stdio.h>

#include <common/base64.h>
#include <common/cs_dbg.h>

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
#define MAX_PLAIN_NAME_LEN \
    ((((SPIFFS_OBJ_NAME_LEN - 1) * 6 / 8) / \
      CS_SPIFFS_ENCRYPTION_BLOCK_SIZE) * \
     CS_SPIFFS_ENCRYPTION_BLOCK_SIZE)

/* file_meta size must match SPIFFS_OBJ_META_LEN. */
typedef char sizeof_file_meta_is_wrong
    [(SPIFFS_OBJ_META_LEN == sizeof(struct file_meta)) ? 1 : -1];

static bool s_names_encrypted = false;

#endif

static const char *drop_dir(const char *fname) {
  const char *old_fname = fname;
  /* Drop "./", if any */
  if (fname[0] == '.' && fname[1] == '/') {
    fname += 2;
  }
  /*
   * Drop / if it is the only one in the path.
   * This allows use of /pretend/directories but serves /file.txt as normal.
   */
  if (fname[0] == '/' && strchr(fname + 1, '/') == NULL) {
    fname++;
  }
  if (fname != old_fname) {
    LOG(LL_DEBUG, ("'%s' -> '%s'", old_fname, fname));
  }
  return fname;
}

static int set_errno(int e) {
  errno = e;
  return (e == 0 ? 0 : -1);
}

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

int set_spiffs_errno(spiffs *fs, const char *op, int res) {
  int e = SPIFFS_errno(fs);
  LOG(LL_DEBUG, ("%s: res = %d, e = %d", op, res, e));
  if (res >= 0) return res;
  return set_errno(spiffs_err_to_errno(e));
}

int spiffs_vfs_open(spiffs *fs, const char *path, int flags, int mode) {
  (void) mode;
  spiffs_mode sm = 0;
  int rw = (flags & 3);
  if (rw == O_RDONLY || rw == O_RDWR) sm |= SPIFFS_RDONLY;
  if (rw == O_WRONLY || rw == O_RDWR) sm |= SPIFFS_WRONLY;
  if (flags & O_CREAT) sm |= SPIFFS_CREAT;
  if (flags & O_TRUNC) sm |= SPIFFS_TRUNC;
#ifdef O_EXCL
  if (flags & O_EXCL) sm |= SPIFFS_EXCL;
#endif

  path = drop_dir(path);

#if CS_SPIFFS_ENABLE_ENCRYPTION
  sm |= SPIFFS_RDONLY;  /* Encryption always needs to be able to read. */

  char enc_path[SPIFFS_OBJ_NAME_LEN];
  if (s_names_encrypted) {
    if (!spiffs_vfs_enc_name(path, enc_path, sizeof(enc_path))) {
      errno = ENXIO;
      return -1;
    }
    path = enc_path;
  }

  int fd = SPIFFS_open(fs, path, sm, 0);
  if (fd >= 0 && (rw & O_WRONLY)) {
    spiffs_stat s;
    s32_t r = SPIFFS_fstat(fs, fd, &s);
    if (r < 0) return set_spiffs_errno(fs, "read", r);
    struct file_meta *fm = (struct file_meta *) s.meta;
    if (fm->plain_size == DEFAULT_PLAIN_SIZE || (flags & O_TRUNC)) {
      /* Can only happen to new files. */
      if (s.size != 0) {
        LOG(LL_ERROR, ("Corrupted encrypted file %s (es %u ps %u)", s.name, s.size, fm->plain_size));
        SPIFFS_close(fs, fd);
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
     * just one flag seems like an overkill. Yes, this may result in additional
     * churn, but should be ok in most cases.
     */
    fm->not_o_append = !(flags & O_APPEND);
    r = SPIFFS_fupdate_meta(fs, fd, fm);
    if (r < 0) {
      set_spiffs_errno(fs, path, fd);
      SPIFFS_close(fs, fd);
      return -1;
    }
  }
  return set_spiffs_errno(fs, path, fd);
#else
  if (flags & O_APPEND) sm |= SPIFFS_APPEND;
  return set_spiffs_errno(fs, path, SPIFFS_open(fs, drop_dir(path), sm, 0));
#endif
}

int spiffs_vfs_close(spiffs *fs, int fd) {
  return set_spiffs_errno(fs, "close", SPIFFS_close(fs, fd));
}

ssize_t spiffs_vfs_read(spiffs *fs, int fd, void *dstv, size_t size) {
#if CS_SPIFFS_ENABLE_ENCRYPTION
  spiffs_stat s;
  s32_t r = SPIFFS_fstat(fs, fd, &s);
  if (r < 0) return set_spiffs_errno(fs, "read", r);
  const struct file_meta *fm = (const struct file_meta *) s.meta;
  LOG(LL_DEBUG, ("enc_read %s (%u) %d", s.name, s.obj_id, size));
  if (!fm->encryption_not_started) {
    if (fm->plain_size == DEFAULT_PLAIN_SIZE || (s.size % CS_SPIFFS_ENCRYPTION_BLOCK_SIZE != 0)) {
      goto out_enc_err;
    }
    s32_t plain_off = SPIFFS_tell(fs, fd);
    if (plain_off < 0) return set_spiffs_errno(fs, "enc_read_tell", plain_off);
    s32_t block_off = (plain_off - (plain_off % CS_SPIFFS_ENCRYPTION_BLOCK_SIZE));
    if (block_off != plain_off) {
      r = SPIFFS_lseek(fs, fd, block_off, SPIFFS_SEEK_SET);
      if (r != block_off) {
        if (r < 0) {
          return set_spiffs_errno(fs, "enc_read_lseek", r);
        } else {
          goto out_enc_err;
        }
      }
    }
    uint8_t *dst = (uint8_t *) dstv;
    int num_read = 0;
    while (num_read < size) {
      uint8_t block[CS_SPIFFS_ENCRYPTION_BLOCK_SIZE];
      r = SPIFFS_read(fs, fd, block, sizeof(block));
      if (r != sizeof(block)) {
        if (r == 0 || SPIFFS_errno(fs) == SPIFFS_ERR_END_OF_OBJECT) {
          break;
        } else {
          if (r < 0) {
            return set_spiffs_errno(fs, "enc_read_read", r);
          } else {
            LOG(LL_ERROR, ("Expected to read %d @ %d, got %d", sizeof(block), block_off, r));
            goto out_enc_err;
          }
        }
      }
      if (!spiffs_vfs_decrypt_block(s.obj_id, block_off, block, sizeof(block))) {
        LOG(LL_ERROR, ("Decryption failed"));
        goto out_enc_err;
      }
      int to_skip = (num_read == 0 ? (plain_off - block_off) : 0);
      int to_copy = (fm->plain_size - block_off);
      if (to_copy > sizeof(block)) to_copy = sizeof(block);
      to_copy -= to_skip;
      if (to_copy > (size - num_read)) to_copy = (size - num_read);
      memcpy(dst, block + to_skip, to_copy);
      LOG(LL_DEBUG, ("enc_read po %d bo %d ts %d tc %d nr %d", plain_off, block_off, to_skip, to_copy, num_read));
      block_off += sizeof(block);
      num_read += to_copy;
      dst += to_copy;
    }
    LOG(LL_DEBUG, ("%d @ %d => %d", size, plain_off, num_read));
    SPIFFS_lseek(fs, fd, plain_off + num_read, SPIFFS_SEEK_SET);
    return num_read;

out_enc_err:
    LOG(LL_ERROR, ("Corrupted encrypted file %s (es %u ps %u)", s.name, s.size, fm->plain_size));
    errno = ENXIO;
    return -1;
  } else
#endif /* CS_SPIFFS_ENABLE_ENCRYPTION */
  {
    int n = SPIFFS_read(fs, fd, dstv, size);
    if (n < 0 && SPIFFS_errno(fs) == SPIFFS_ERR_END_OF_OBJECT) {
      /* EOF */
      n = 0;
    }
    return set_spiffs_errno(fs, "read", n);
  }
}

size_t spiffs_vfs_write(spiffs *fs, int fd, const void *datav, size_t size) {
#if CS_SPIFFS_ENABLE_ENCRYPTION
  spiffs_stat s;
  s32_t r = SPIFFS_fstat(fs, fd, &s);
  if (r < 0) return set_spiffs_errno(fs, "read", r);
  struct file_meta *fm = (struct file_meta *) s.meta;
  if (!fm->encryption_not_started) {
    if (fm->plain_size == DEFAULT_PLAIN_SIZE || (s.size % CS_SPIFFS_ENCRYPTION_BLOCK_SIZE != 0)) {
      goto out_enc_err;
    }
    s32_t plain_off = SPIFFS_tell(fs, fd);
    if (plain_off < 0) return set_spiffs_errno(fs, "enc_write_tell", plain_off);
    if (!(fm->not_o_append)) plain_off = fm->plain_size;
    s32_t block_off = (plain_off - (plain_off % CS_SPIFFS_ENCRYPTION_BLOCK_SIZE));
    uint8_t block[CS_SPIFFS_ENCRYPTION_BLOCK_SIZE];
    int prefix_len = plain_off - block_off;
    if (prefix_len > 0) {
      r = SPIFFS_lseek(fs, fd, block_off, SPIFFS_SEEK_SET);
      if (r != block_off) {
        if (r < 0) {
          return set_spiffs_errno(fs, "enc_write_lseek", r);
        } else {
          goto out_enc_err;
        }
      }
      r = SPIFFS_read(fs, fd, block, prefix_len);
      if (r != prefix_len) {
        if (r < 0) {
          return set_spiffs_errno(fs, "enc_write_read", r);
        } else {
          LOG(LL_ERROR, ("Expected to read %d @ %d, got %d", prefix_len, block_off, r));
          goto out_enc_err;
        }
      }
      if (!spiffs_vfs_decrypt_block(s.obj_id, block_off, block, sizeof(block))) {
        LOG(LL_ERROR, ("Decryption failed"));
        goto out_enc_err;
      }
      r = SPIFFS_lseek(fs, fd, block_off, SPIFFS_SEEK_SET);
      if (r != block_off) {
        if (r < 0) {
          return set_spiffs_errno(fs, "enc_write_lseek2", r);
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
      if (!spiffs_vfs_encrypt_block(s.obj_id, block_off, block, sizeof(block))) {
        LOG(LL_ERROR, ("Encryption failed"));
        if (num_written > 0) break;
        goto out_enc_err;
      }
      r = SPIFFS_write(fs, fd, block, sizeof(block));
      if (r != sizeof(block)) {
        if (num_written > 0) break;
        if (r < 0) {
          return set_spiffs_errno(fs, "enc_write_write", r);
        } else {
          LOG(LL_ERROR, ("Expected to write %d @ %d, got %d", sizeof(block), block_off, r));
          goto out_enc_err;
        }
      }
      LOG(LL_DEBUG, ("enc_write s %d po %d bo %d ts %d tc %d nw %d", size, plain_off, block_off, to_skip, to_copy, num_written));
      block_off += sizeof(block);
      num_written += to_copy;
      data += to_copy;
      to_skip = 0;
    }
    /* We have written some data, calls below should not cause failure. */
    int new_plain_off = plain_off + num_written;
    if (new_plain_off > fm->plain_size) {
      fm->plain_size = new_plain_off;
      r = SPIFFS_fupdate_meta(fs, fd, fm);
      if (r < 0) {
        LOG(LL_ERROR, ("enc_write_update_meta: %d", r));
      }
    }
    LOG(LL_DEBUG, ("%d @ %d => %d, po %d ps %d", size, plain_off, num_written, new_plain_off, (int) fm->plain_size));
    SPIFFS_lseek(fs, fd, new_plain_off, SPIFFS_SEEK_SET);
    return num_written;

out_enc_err:
    LOG(LL_ERROR, ("Corrupted encrypted file %s (es %u ps %u)", s.name, s.size, fm->plain_size));
    errno = ENXIO;
    return -1;
  } else
#endif /* CS_SPIFFS_ENABLE_ENCRYPTION */
  {
    return set_spiffs_errno(fs, "write",
                            SPIFFS_write(fs, fd, (void *) datav, size));
  }
}

static void spiffs_vfs_xlate_stat(spiffs_stat *ss, struct stat *st) {
  st->st_ino = ss->obj_id;
  st->st_mode = S_IFREG | 0666;
  st->st_nlink = 1;
  st->st_size = ss->size;
#if CS_SPIFFS_ENABLE_ENCRYPTION
  const struct file_meta *fm = (const struct file_meta *) ss->meta;
  if (fm->plain_size != DEFAULT_PLAIN_SIZE) {
    st->st_size = fm->plain_size;
  }
#endif
}

int spiffs_vfs_stat(spiffs *fs, const char *path, struct stat *st) {
  int res;
  spiffs_stat ss;
  memset(st, 0, sizeof(*st));
  const char *fname = drop_dir(path);
  /* Simulate statting the root directory. */
  if (fname[0] == '\0' || strcmp(fname, ".") == 0) {
    st->st_ino = 0;
    st->st_mode = S_IFDIR | 0777;
    st->st_nlink = 1;
    st->st_size = 0;
    return set_spiffs_errno(fs, path, SPIFFS_OK);
  }
#if CS_SPIFFS_ENABLE_ENCRYPTION
  char enc_fname[SPIFFS_OBJ_NAME_LEN];
  if (s_names_encrypted) {
    if (!spiffs_vfs_enc_name(fname, enc_fname, sizeof(enc_fname))) {
      errno = ENXIO;
      return -1;
    }
  }
  fname = enc_fname;
#endif
  res = SPIFFS_stat(fs, fname, &ss);
  if (res == SPIFFS_OK) {
    spiffs_vfs_xlate_stat(&ss, st);
  }
  return set_spiffs_errno(fs, path, res);
}

int spiffs_vfs_fstat(spiffs *fs, int fd, struct stat *st) {
  int res;
  spiffs_stat ss;
  memset(st, 0, sizeof(*st));
  res = SPIFFS_fstat(fs, fd, &ss);
  if (res == SPIFFS_OK) {
    spiffs_vfs_xlate_stat(&ss, st);
  }
  return set_spiffs_errno(fs, "fstat", res);
}

off_t spiffs_vfs_lseek(spiffs *fs, int fd, off_t offset, int whence) {
#if CS_SPIFFS_ENABLE_ENCRYPTION
  spiffs_stat s;
  s32_t r = SPIFFS_fstat(fs, fd, &s);
  if (r < 0) return set_spiffs_errno(fs, "lseek", r);
  const struct file_meta *fm = (const struct file_meta *) s.meta;
  if (fm->plain_size != DEFAULT_PLAIN_SIZE) {
    off_t cur = SPIFFS_tell(fs, fd);
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
  return set_spiffs_errno(fs, "lseek", SPIFFS_lseek(fs, fd, offset, whence));
}

int spiffs_vfs_rename(spiffs *fs, const char *src, const char *dst) {
  int res;
  src = drop_dir(src);
  dst = drop_dir(dst);
#if CS_SPIFFS_ENABLE_ENCRYPTION
  char enc_src[SPIFFS_OBJ_NAME_LEN], enc_dst[SPIFFS_OBJ_NAME_LEN];
  if (s_names_encrypted) {
    if (!spiffs_vfs_enc_name(src, enc_src, sizeof(enc_src)) ||
        !spiffs_vfs_enc_name(dst, enc_dst, sizeof(enc_dst))) {
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
    res = SPIFFS_stat(fs, dst, &ss);
    if (res == 0) {
      SPIFFS_remove(fs, dst);
    }
  }
  return set_spiffs_errno(fs, "rename", SPIFFS_rename(fs, src, dst));
}

int spiffs_vfs_unlink(spiffs *fs, const char *path) {
  path = drop_dir(path);
#if CS_SPIFFS_ENABLE_ENCRYPTION
  char enc_path[SPIFFS_OBJ_NAME_LEN];
  if (s_names_encrypted) {
    if (!spiffs_vfs_enc_name(path, enc_path, sizeof(enc_path))) {
      errno = ENXIO;
      return -1;
    }
    path = enc_path;
  }
#endif
  return set_spiffs_errno(fs, "unlink", SPIFFS_remove(fs, path));
}

struct spiffs_dir {
  DIR dir;
  spiffs_DIR sdh;
  struct spiffs_dirent sde;
  struct dirent de;
};

DIR *spiffs_vfs_opendir(spiffs *fs, const char *name) {
  struct spiffs_dir *sd = NULL;

  if (name == NULL) {
    errno = EINVAL;
    return NULL;
  }

  if ((sd = (struct spiffs_dir *) calloc(1, sizeof(*sd))) == NULL) {
    errno = ENOMEM;
    return NULL;
  }

  if (SPIFFS_opendir(fs, name, &sd->sdh) == NULL) {
    free(sd);
    sd = NULL;
    errno = EINVAL;
  }

  return (DIR *) sd;
}

struct dirent *spiffs_vfs_readdir(spiffs *fs, DIR *dir) {
  struct spiffs_dir *sd = (struct spiffs_dir *) dir;
  if (SPIFFS_readdir(&sd->sdh, &sd->sde) == SPIFFS_OK) {
    errno = EBADF;
    return NULL;
  }
  sd->de.d_ino = sd->sde.obj_id;
#if CS_SPIFFS_ENABLE_ENCRYPTION
  if (s_names_encrypted) {
    if (!spiffs_vfs_dec_name((const char *) sd->sde.name, sd->de.d_name,
                             sizeof(sd->de.d_name))) {
      LOG(LL_ERROR, ("Name decryption failed (%s)", sd->sde.name));
      errno = ENXIO;
      return NULL;
    }
  }
#else
  memcpy(sd->de.d_name, sd->sde.name, SPIFFS_OBJ_NAME_LEN);
#endif
  (void) fs;
  return &sd->de;
}

int spiffs_vfs_closedir(spiffs *fs, DIR *dir) {
  struct spiffs_dir *sd = (struct spiffs_dir *) dir;
  if (dir != NULL) {
    SPIFFS_closedir(&sd->sdh);
    free(dir);
  }
  (void) fs;
  return 0;
}

#if CS_SPIFFS_ENABLE_ENCRYPTION
bool spiffs_vfs_enc_name(const char *name, char *enc_name, size_t enc_name_size) {
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
    if (!spiffs_vfs_encrypt_block(0, enc_name_len, tmp + enc_name_len, CS_SPIFFS_ENCRYPTION_BLOCK_SIZE)) {
      return false;
    }
    enc_name_len += CS_SPIFFS_ENCRYPTION_BLOCK_SIZE;
  }
  cs_base64_encode(tmp, enc_name_len, tmp2);  /* NUL-terminates output. */
  LOG(LL_DEBUG, ("%s -> %s", name, tmp2));
  strncpy(enc_name, tmp2, enc_name_size);
  return true;
}

bool spiffs_vfs_dec_name(const char *enc_name, char *name, size_t name_size) {
  int i;
  char tmp[SPIFFS_OBJ_NAME_LEN];
  int enc_name_len = 0;
  cs_base64_decode((const unsigned char *) enc_name, strlen(enc_name), tmp, &enc_name_len);
  if (enc_name_len == 0 || (enc_name_len % CS_SPIFFS_ENCRYPTION_BLOCK_SIZE != 0)) {
    return false;
  }
  for (i = 0; i < enc_name_len; i += CS_SPIFFS_ENCRYPTION_BLOCK_SIZE) {
    if (!spiffs_vfs_decrypt_block(0, i, tmp + i, CS_SPIFFS_ENCRYPTION_BLOCK_SIZE)) {
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

bool spiffs_vfs_enc_fs(spiffs *fs) {
  bool res = false;
  spiffs_DIR d;
  struct spiffs_dirent e;
  spiffs_file fd = -1;
  if (SPIFFS_opendir(fs, "/", &d) == NULL) {
    return false;
  }
  while (SPIFFS_readdir(&d, &e) != NULL) {
    char enc_name[SPIFFS_OBJ_NAME_LEN];
    struct file_meta *fm = (struct file_meta *) e.meta;
    LOG(LL_DEBUG, ("%s (%u) es %u ps %u", e.name, e.obj_id, e.size, fm->plain_size));
    if (fm->plain_size != DEFAULT_PLAIN_SIZE) continue;  /* Already encrypted */
    if (!fm->encryption_not_started) {
      LOG(LL_ERROR, ("%s is half-encrypted; FS is corrupted."));
      continue;
    }
    if (!spiffs_vfs_enc_name((const char *) e.name, enc_name, sizeof(enc_name))) {
      LOG(LL_ERROR, ("%s: name encryption failed", e.name));
      goto out;
    }
    LOG(LL_INFO, ("Encrypting %s (id %u, size %d) -> %s", e.name, e.obj_id, (int) e.size, enc_name));
    fd = SPIFFS_open_by_dirent(fs, &e, SPIFFS_RDWR, 0);
    if (fd < 0) {
      LOG(LL_ERROR, ("%s: open failed: %d", e.name, SPIFFS_errno(fs)));
      goto out;
    }
    fm->encryption_not_started = false;
    if (SPIFFS_fupdate_meta(fs, fd, fm) != SPIFFS_OK) {
      LOG(LL_ERROR, ("%s: update_meta failed: %d", e.name, SPIFFS_errno(fs)));
      goto out;
    }
    int enc_size = 0;
    fm->plain_size = 0;
    while (true) {
      uint8_t block[CS_SPIFFS_ENCRYPTION_BLOCK_SIZE];
      int n = SPIFFS_read(fs, fd, block, sizeof(block));
      if (n < 0) {
        int err = SPIFFS_errno(fs);
        if (err == SPIFFS_ERR_END_OF_OBJECT) break;
        LOG(LL_ERROR, ("%s: read failed: %d", e.name, err));
        goto out;
      } else if (n == 0) {
        break;
      }
      for (int i = n; i < sizeof(block); i++) {
        block[i] = 0;
      }
      if (!spiffs_vfs_encrypt_block(e.obj_id, fm->plain_size, block, sizeof(block))) {
        LOG(LL_ERROR, ("%s: encrypt failed", e.name));
        goto out;
      }
      if (SPIFFS_lseek(fs, fd, fm->plain_size, SPIFFS_SEEK_SET) != fm->plain_size) {
        LOG(LL_ERROR, ("%s: seek failed: %d", e.name, SPIFFS_errno(fs)));
        goto out;
      }
      int r = SPIFFS_write(fs, fd, block, sizeof(block));
      if (r != sizeof(block)) {
        LOG(LL_ERROR, ("%s: write failed: %d", e.name, SPIFFS_errno(fs)));
        goto out;
      }
      fm->plain_size += n;
      enc_size += sizeof(block);
    }
    if (SPIFFS_fupdate_meta(fs, fd, fm) != SPIFFS_OK) {
      LOG(LL_ERROR, ("%s: final update_meta failed: %d", e.name, SPIFFS_errno(fs)));
      goto out;
    }
    SPIFFS_close(fs, fd);
    fd = -1;
    if (SPIFFS_rename(fs, (const char *) e.name, enc_name) != SPIFFS_OK) {
      LOG(LL_ERROR, ("%s: rename failed: %d", e.name, SPIFFS_errno(fs)));
      goto out;
    }
    /*
     * We have to start over, readdir can get confused if FS is mutated
     * while iterating.
     */
    SPIFFS_closedir(&d);
    if (SPIFFS_opendir(fs, "/", &d) == NULL) {
      return false;
    }
  }
  s_names_encrypted = true;
  res = true;
out:
  if (fd >= 0) SPIFFS_close(fs, fd);
  SPIFFS_closedir(&d);
  return res;
}
#endif /* CS_SPIFFS_ENABLE_ENCRYPTION */

#endif /* CS_SPIFFS_ENABLE_VFS */
