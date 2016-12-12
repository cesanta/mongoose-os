/* Low-level FS interface, SPIFFS-on-FailFS container. */

#include "cc3200_fs_spiffs_container.h"
#include "cc3200_fs_spiffs_container_meta.h"

#include <stdlib.h>

#include "hw_types.h"
#include "simplelink.h"
#include "device.h"
#include "fs.h"

#include "common/cs_dbg.h"
#include "fw/src/miot_hal.h"

#include "fw/platforms/cc3200/boot/lib/boot.h"

#include "config.h"
#include "spiffs_nucleus.h"

#include "mongoose/mongoose.h"

#define MAX_FS_CONTAINER_FNAME_LEN (MAX_FS_CONTAINER_PREFIX_LEN + 3)
void fs_container_fname(const char *cpfx, int cidx, _u8 *fname) {
  int l = 0;
  while (cpfx[l] != '\0' && l < MAX_FS_CONTAINER_PREFIX_LEN) {
    fname[l] = cpfx[l];
    l++;
  }
  fname[l++] = '.';
  fname[l++] = '0' + cidx;
  fname[l] = '\0';
}

_i32 fs_create_container(const char *cpfx, int cidx, _u32 fs_size) {
  _i32 fh = -1;
  _u8 fname[MAX_FS_CONTAINER_FNAME_LEN];
  _u32 fsize = FS_CONTAINER_SIZE(fs_size);
  fs_container_fname(cpfx, cidx, fname);
  int r = sl_FsDel(fname, 0);
  DBG(("del %s -> %d", fname, r));
  r = sl_FsOpen(fname, FS_MODE_OPEN_CREATE(fsize, 0), NULL, &fh);
  LOG((r == 0 ? LL_DEBUG : LL_ERROR),
      ("open %s %d -> %d %d", fname, (int) fsize, (int) r, (int) fh));
  if (r != 0) return r;
  return fh;
}

_i32 fs_delete_container(const char *cpfx, int cidx) {
  _i32 ret = -1;
  SlFsFileInfo_t fi;
  _u8 fname[MAX_FS_CONTAINER_FNAME_LEN];
  fs_container_fname(cpfx, cidx, fname);
  ret = sl_FsGetInfo(fname, 0, &fi);
  if (ret == 0) {
    LOG(LL_INFO, ("Deleting %s", fname));
    ret = sl_FsDel(fname, 0);
  }
  return ret;
}

_i32 fs_write_meta(_i32 fh, _u64 seq, _u32 fs_size, _u32 fs_block_size,
                   _u32 fs_page_size, _u32 fs_erase_size) {
  int r;
  _u32 offset;
  union fs_container_meta meta;
  memset(&meta, 0xff, sizeof(meta));
  meta.info.seq = seq;
  meta.info.fs_size = fs_size;
  meta.info.fs_block_size = fs_block_size;
  meta.info.fs_page_size = fs_page_size;
  meta.info.fs_erase_size = fs_erase_size;

  offset = FS_CONTAINER_SIZE(meta.info.fs_size) - sizeof(meta);
  r = sl_FsWrite(fh, offset, (_u8 *) &meta, sizeof(meta));
  DBG(("write meta %llu @ %d: %d", seq, (int) offset, (int) r));
  if (r == sizeof(meta)) r = 0;
  return r;
}

static _i32 fs_write_mount_meta(struct mount_info *m) {
  return fs_write_meta(m->fh, m->seq, m->fs.cfg.phys_size,
                       m->fs.cfg.log_block_size, m->fs.cfg.log_page_size,
                       m->fs.cfg.phys_erase_block);
}

static _u8 *get_buf(_u32 *size) {
  _u8 *buf = NULL;
  while (*size > 0) {
    buf = malloc(*size);
    if (buf != NULL) break;
    *size /= 2;
  }
  return buf;
}

static _i32 fs_switch_container(struct mount_info *m, _u32 mask_begin,
                                _u32 mask_len) {
  int r;
  int new_cidx = m->cidx ^ 1;
  _i32 old_fh = m->fh, new_fh;
  _u8 *buf;
  _u32 offset, len, buf_size;
  LOG(LL_DEBUG, ("%s %d -> %d", m->cpfx, m->cidx, new_cidx));
  if (old_fh > 0 && m->rw) {
    /*
     * During the switch the destination container will be unusable.
     * If switching from a writeable container (likely in response to an erase),
     * close the old container first to make it safe and reopen for reading.
     */
    fs_close_container(m);
    old_fh = -1;
  }
  if (old_fh < 0) {
    _u8 fname[MAX_FS_CONTAINER_FNAME_LEN];
    fs_container_fname(m->cpfx, m->cidx, fname);
    r = sl_FsOpen(fname, FS_MODE_OPEN_READ, NULL, &old_fh);
    DBG(("fopen %s %d", m->cpfx, r));
    if (r < 0) {
      r = SPIFFS_ERR_NOT_READABLE;
      goto out_close_old;
    }
  }
  miot_wdt_feed();
  new_fh = fs_create_container(m->cpfx, new_cidx, m->fs.cfg.phys_size);
  if (new_fh < 0) {
    r = new_fh;
    goto out_close_old;
  }

  buf_size = 1024;
  buf = get_buf(&buf_size);
  if (buf == NULL) {
    r = SPIFFS_ERR_INTERNAL;
    goto out_close_new;
  }

  for (offset = 0; offset < m->fs.cfg.phys_size;) {
    len = buf_size;
    if (offset == mask_begin) {
      offset = mask_begin + mask_len;
    } else if (offset + len > mask_begin && offset < mask_begin + mask_len) {
      len = mask_begin - offset;
    }
    if (offset + len > m->fs.cfg.phys_size) {
      len = m->fs.cfg.phys_size - offset;
    }
    DBG(("copy %d @ %d", (int) len, (int) offset));
    if (len > 0) {
      r = sl_FsRead(old_fh, offset, buf, len);
      if (r != len) {
        r = SPIFFS_ERR_NOT_READABLE;
        goto out_free;
      }
      r = sl_FsWrite(new_fh, offset, buf, len);
      if (r != len) {
        r = SPIFFS_ERR_NOT_WRITABLE;
        goto out_free;
      }
      offset += len;
    }
  }

  m->seq--;
  m->cidx = new_cidx;
  m->fh = new_fh;
  new_fh = -1;
  m->rw = 1;

  r = fs_write_mount_meta(m);

  m->last_write = mg_time();

out_free:
  free(buf);
out_close_new:
  if (new_fh > 0) sl_FsClose(new_fh, NULL, NULL, 0);
out_close_old:
  sl_FsClose(old_fh, NULL, NULL, 0);
  LOG((r == 0 ? LL_DEBUG : LL_ERROR), ("%d", r));
  return r;
}

void fs_close_container(struct mount_info *m) {
  if (!m->valid || m->fh == -1) return;
  LOG(LL_DEBUG, ("fh %d", (int) m->fh));
  sl_FsClose(m->fh, NULL, NULL, 0);
  m->fh = -1;
  m->rw = 0;
}

static s32_t failfs_read(spiffs *fs, u32_t addr, u32_t size, u8_t *dst) {
  struct mount_info *m = (struct mount_info *) fs->user_data;
  _i32 r;
  DBG(("failfs_read %d @ %d, cidx %u # %llu, fh %d, valid %d, rw %d",
       (int) size, (int) addr, m->cidx, m->seq, (int) m->fh, m->valid, m->rw));
  if (!m->valid) return SPIFFS_ERR_NOT_READABLE;
  do {
    if (m->fh < 0) {
      _u8 fname[MAX_FS_CONTAINER_FNAME_LEN];
      fs_container_fname(m->cpfx, m->cidx, fname);
      r = sl_FsOpen(fname, FS_MODE_OPEN_READ, NULL, &m->fh);
      DBG(("fopen %d", (int) r));
      if (r < 0) return SPIFFS_ERR_NOT_READABLE;
    }
    r = sl_FsRead(m->fh, addr, dst, size);
    DBG(("read %d", (int) r));
    if (r == SL_FS_ERR_INVALID_HANDLE) {
      /*
       * This happens when SimpleLink is reinitialized - all file handles are
       * invalidated. SL has to be reinitialized to e.g. configure WiFi.
       */
      m->fh = -1;
    }
  } while (m->fh < 0);
  return (r == size) ? SPIFFS_OK : SPIFFS_ERR_NOT_READABLE;
}

static s32_t failfs_write(spiffs *fs, u32_t addr, u32_t size, u8_t *src) {
  struct mount_info *m = (struct mount_info *) fs->user_data;
  _i32 r;
  DBG(("failfs_write %d @ %d, cidx %d # %llu, fh %d, valid %d, rw %d",
       (int) size, (int) addr, m->cidx, m->seq, (int) m->fh, m->valid, m->rw));
  if (!m->valid) return SPIFFS_ERR_NOT_WRITABLE;
  if (!m->rw) {
    /* Remount rw. */
    if (fs_switch_container(m, 0, 0) != 0) return SPIFFS_ERR_NOT_WRITABLE;
  }
  r = sl_FsWrite(m->fh, addr, src, size);
  DBG(("write %d", (int) r));
  m->last_write = mg_time();
  return (r == size) ? SPIFFS_OK : SPIFFS_ERR_NOT_WRITABLE;
}

static s32_t failfs_erase(spiffs *fs, u32_t addr, u32_t size) {
  struct mount_info *m = (struct mount_info *) fs->user_data;
  DBG(("failfs_erase %d @ %d", (int) size, (int) addr));
  return fs_switch_container(m, addr, size) == 0 ? SPIFFS_OK
                                                 : SPIFFS_ERR_ERASE_FAIL;
}

static int fs_get_info(const char *cpfx, int cidx,
                       struct fs_container_info *info) {
  _u8 fname[MAX_FS_CONTAINER_FNAME_LEN];
  union fs_container_meta meta;
  SlFsFileInfo_t fi;
  _i32 fh;
  _u32 offset;
  fs_container_fname(cpfx, cidx, fname);
  _i32 r = sl_FsGetInfo(fname, 0, &fi);
  DBG(("finfo %s %d %d %d", fname, (int) r, (int) fi.FileLen,
       (int) fi.AllocatedLen));
  if (r != 0) return r;
  if (fi.AllocatedLen < sizeof(meta)) return -200;
  r = sl_FsOpen(fname, FS_MODE_OPEN_READ, NULL, &fh);
  DBG(("fopen %s %d", fname, (int) r));
  if (r != 0) return r;

  offset = fi.FileLen - sizeof(meta);
  r = sl_FsRead(fh, offset, (_u8 *) &meta, sizeof(meta));
  DBG(("read meta @ %d: %d", (int) offset, (int) r));

  if (r != sizeof(meta)) {
    r = -201;
    goto out_close;
  }
  if (meta.info.seq > FS_INITIAL_SEQ || meta.info.seq == 0 ||
      meta.info.fs_size == ~0UL) {
    r = -202;
    goto out_close;
  }

  memcpy(info, &meta.info, sizeof(*info));
  DBG(("found fs: %llu %d %d %d %d", info->seq, (int) info->fs_size,
       (int) info->fs_block_size, (int) info->fs_page_size,
       (int) info->fs_erase_size));
  r = 0;

out_close:
  sl_FsClose(fh, NULL, NULL, 0);
  return r;
}

static _i32 fs_mount_spiffs(struct mount_info *m, _u32 fs_size, _u32 block_size,
                            _u32 page_size, _u32 erase_size) {
  int r;
  spiffs_config cfg;
  cfg.hal_read_f = failfs_read;
  cfg.hal_write_f = failfs_write;
  cfg.hal_erase_f = failfs_erase;
  cfg.phys_size = fs_size;
  cfg.phys_addr = 0;
  cfg.phys_erase_block = erase_size;
  cfg.log_block_size = block_size;
  cfg.log_page_size = page_size;
  m->work = calloc(2, page_size);
  m->fds = calloc(1, MAX_OPEN_SPIFFS_FILES * sizeof(spiffs_fd));
  m->fs.user_data = m;
  r = SPIFFS_mount(&m->fs, &cfg, m->work, m->fds,
                   MAX_OPEN_SPIFFS_FILES * sizeof(spiffs_fd), NULL, 0, NULL);
  if (r != SPIFFS_OK) {
    free(m->work);
    free(m->fds);
    m->work = m->fds = NULL;
  }
  return r;
}

static int fs_mount_idx(const char *cpfx, int cidx, struct mount_info *m) {
  int r;
  struct fs_container_info fsi;
  memset(m, 0, sizeof(*m));
  m->fh = -1;
  m->cidx = cidx;
  r = fs_get_info(cpfx, cidx, &fsi);
  if (r != 0) return r;
  m->cpfx = strdup(cpfx);
  m->seq = fsi.seq;
  m->valid = 1;
  LOG(LL_INFO, ("Mounting %s.%d 0x%llx", cpfx, cidx, fsi.seq));
  r = fs_mount_spiffs(m, fsi.fs_size, fsi.fs_block_size, fsi.fs_page_size,
                      fsi.fs_erase_size);
  DBG(("mount %d: %d %d", cidx, (int) r, (int) SPIFFS_errno(&m->fs)));
  if (r < 0) {
    LOG(LL_ERROR, ("Mount failed: %d", r));
  }
  return r;
}

static int fs_get_active_idx(const char *cpfx) {
  struct fs_container_info fs0, fs1;
  int r0 = fs_get_info(cpfx, 0, &fs0);
  int r1 = fs_get_info(cpfx, 1, &fs1);

  DBG(("r0 = %d %llx, r1 = %d %llx", r0, fs0.seq, r1, fs1.seq));

  if (r0 == 0 && r1 == 0) {
    if (fs0.seq < fs1.seq) {
      r1 = -1;
    } else {
      r0 = -1;
    }
  }
  if (r0 == 0) return 0;
  if (r1 == 0) return 1;
  return -1;
}

_i32 fs_mount(const char *cpfx, struct mount_info *m) {
  int active_idx = fs_get_active_idx(cpfx);
  if (active_idx < 0) return -1000;
  return fs_mount_idx(cpfx, active_idx, m);
}

_i32 fs_umount(struct mount_info *m) {
  LOG(LL_INFO, ("Unmounting %s.%d", m->cpfx, m->cidx));
  SPIFFS_unmount(&m->fs);
  free(m->cpfx);
  free(m->work);
  free(m->fds);
  fs_close_container(m);
  memset(m, 0, sizeof(*m));
  return 1;
}

_i32 fs_delete_inactive_container(const char *cpfx) {
  int active_idx = fs_get_active_idx(cpfx);
  if (active_idx < 0) return -1000;
  int inactive_idx = (active_idx == 0 ? 1 : 0);
  return fs_delete_container(cpfx, inactive_idx);
}
