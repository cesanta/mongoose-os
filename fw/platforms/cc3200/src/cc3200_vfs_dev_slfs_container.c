/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/cc3200/src/cc3200_vfs_dev_slfs_container.h"

#include "common/cs_dbg.h"
#include "common/platform.h"
#include "common/queue.h"

#include "frozen/frozen.h"
#include "mongoose/mongoose.h"

#include "mgos_hal.h"
#include "mgos_timers.h"
#include "mgos_vfs_dev.h"
#include "mgos_vfs_fs_spiffs.h"

#include "fw/platforms/cc3200/boot/lib/boot.h"
#include "fw/platforms/cc3200/src/cc3200_vfs_dev_slfs_container_meta.h"

struct dev_data {
  char *cpfx;        /* Container filename prefix. */
  _u32 size;         /* Size of the container */
  _i32 fh;           /* SLFS file handle, or -1 if not open yet. */
  uint64_t seq;      /* Sequence counter for the mounted container. */
  uint32_t cidx : 1; /* Which of the two containers is currently mounted. */
  uint32_t rw : 1;   /* 1 if the underlying fh is r/w. */
  double last_write; /* Last time container was written (systick) */
  int flush_interval_ms;
  mgos_timer_id flush_timer_id;
  SLIST_ENTRY(dev_data) next;
};

static SLIST_HEAD(s_devs, dev_data) s_devs = SLIST_HEAD_INITIALIZER(s_devs);

#ifndef MGOS_VFS_DEV_SLFS_CONTAINER_FLUSH_INTERVAL_MS
#define MGOS_VFS_DEV_SLFS_CONTAINER_FLUSH_INTERVAL_MS 1000
#endif

void cc3200_vfs_dev_slfs_container_fname(const char *cpfx, int cidx,
                                         _u8 *fname) {
  int l = 0;
  while (cpfx[l] != '\0' && l < MAX_FS_CONTAINER_PREFIX_LEN) {
    fname[l] = cpfx[l];
    l++;
  }
  fname[l++] = '.';
  fname[l++] = '0' + cidx;
  fname[l] = '\0';
}

int fs_get_info(const char *cpfx, int cidx, struct fs_container_info *info) {
  _u8 fname[MAX_FS_CONTAINER_FNAME_LEN];
  union fs_container_meta meta;
  SlFsFileInfo_t fi;
  _i32 fh;
  _u32 offset;
  cc3200_vfs_dev_slfs_container_fname(cpfx, cidx, fname);
  _i32 r = sl_FsGetInfo(fname, 0, &fi);
  DBG(("finfo %s %d %d %d", fname, (int) r, (int) fi.FileLen,
       (int) fi.AllocatedLen));
  if (r != 0) {
    if (r == SL_FS_FILE_HAS_NOT_BEEN_CLOSE_CORRECTLY) {
      LOG(LL_ERROR, ("corrupt container %s", fname));
      sl_FsDel(fname, 0);
    }
    return r;
  }
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
  r = 0;

out_close:
  sl_FsClose(fh, NULL, NULL, 0);
  return r;
}

int fs_get_active_idx(const char *cpfx, struct fs_container_info *info) {
  struct fs_container_info fs0, fs1;
  int r = -1;
  int r0 = fs_get_info(cpfx, 0, &fs0);
  int r1 = fs_get_info(cpfx, 1, &fs1);

  if (r0 == 0 && r1 == 0) {
    if (fs0.seq < fs1.seq) {
      r1 = -1;
    } else {
      r0 = -1;
    }
  }
  if (r0 == 0) {
    *info = fs0;
    r = 0;
  } else if (r1 == 0) {
    *info = fs1;
    r = 1;
  }

  LOG(LL_DEBUG, ("r0 = %d 0x%llx, r1 = %d 0x%llx => %d 0x%llx", r0, fs0.seq, r1,
                 fs1.seq, r, info->seq));

  return r;
}

bool cc3200_vfs_dev_slfs_container_write_meta(int fh, uint64_t seq,
                                              uint32_t fs_size,
                                              uint32_t fs_block_size,
                                              uint32_t fs_page_size,
                                              uint32_t fs_erase_size) {
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
  DBG(("write meta fh %d 0x%llx @ %d: %d", fh, seq, (int) offset, (int) r));
  if (r == sizeof(meta)) r = 0;
  return (r == 0);
}

static bool fs_write_mount_meta(struct dev_data *m) {
  return cc3200_vfs_dev_slfs_container_write_meta(
      m->fh, m->seq, m->size,
      /* These are no longer used, only for backward compat. */
      MGOS_SPIFFS_DEFAULT_BLOCK_SIZE, MGOS_SPIFFS_DEFAULT_PAGE_SIZE,
      MGOS_SPIFFS_DEFAULT_ERASE_SIZE);
}

_i32 fs_open_container(const char *cpfx, int cidx) {
  _u8 fname[MAX_FS_CONTAINER_FNAME_LEN];
  cc3200_vfs_dev_slfs_container_fname(cpfx, cidx, fname);
  _i32 fh;
  _i32 r = sl_FsOpen(fname, FS_MODE_OPEN_READ, NULL, &fh);
  LOG((r >= 0 ? LL_DEBUG : LL_ERROR), ("open %s -> %ld %ld", fname, r, fh));
  return (r >= 0 ? fh : -1);
}

_i32 fs_create_container(const char *cpfx, int cidx, _u32 size) {
  _i32 fh = -1;
  _u8 fname[MAX_FS_CONTAINER_FNAME_LEN];
  _u32 fsize = FS_CONTAINER_SIZE(size);
  cc3200_vfs_dev_slfs_container_fname(cpfx, cidx, fname);
  int r = sl_FsDel(fname, 0);
  DBG(("del %s -> %ld", fname, r));
  r = sl_FsOpen(fname, FS_MODE_OPEN_CREATE(fsize, 0), NULL, &fh);
  LOG((r == 0 ? LL_DEBUG : LL_ERROR),
      ("open %s %lu -> %d %ld", fname, fsize, r, fh));
  if (r != 0) return r;
  return fh;
}

void fs_close_container(struct dev_data *dd) {
  if (dd->fh < 0) return;
  LOG(LL_DEBUG, ("fh %ld rw %d", dd->fh, dd->rw));
  sl_FsClose(dd->fh, NULL, NULL, 0);
  dd->fh = -1;
  dd->rw = false;
  if (dd->flush_timer_id != MGOS_INVALID_TIMER_ID) {
    mgos_clear_timer(dd->flush_timer_id);
    dd->flush_timer_id = MGOS_INVALID_TIMER_ID;
  }
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

static void fs_flush_timer_cb(void *arg) {
  struct dev_data *dd = (struct dev_data *) arg;
  if (dd->rw && mg_time() > dd->last_write + dd->flush_interval_ms / 1000.0) {
    fs_close_container(dd);
  }
}

_i32 fs_switch_container(struct dev_data *dd, _u32 mask_begin, _u32 mask_len) {
  int r;
  int new_cidx = dd->cidx ^ 1;
  _i32 old_fh = dd->fh, new_fh;
  _u8 *buf;
  _u32 offset, len, buf_size;
  LOG(LL_DEBUG, ("%lu %lu %d %s %d -> %d", mask_begin, mask_len, dd->rw,
                 dd->cpfx, dd->cidx, new_cidx));
  if (old_fh > 0 && dd->rw) {
    /*
     * During the switch the destination container will be unusable.
     * If switching from a writeable container (likely in response to an erase),
     * close the old container first to make it safe and reopen for reading.
     */
    fs_close_container(dd);
    old_fh = -1;
  }
  if (old_fh < 0) {
    _u8 fname[MAX_FS_CONTAINER_FNAME_LEN];
    cc3200_vfs_dev_slfs_container_fname(dd->cpfx, dd->cidx, fname);
    r = sl_FsOpen(fname, FS_MODE_OPEN_READ, NULL, &old_fh);
    DBG(("fopen %s %ld", dd->cpfx, r));
    if (r < 0) {
      r = -1;
      goto out_close_old;
    }
  }
  mgos_wdt_feed();
  new_fh = fs_create_container(dd->cpfx, new_cidx, dd->size);
  if (new_fh < 0) {
    r = new_fh;
    goto out_close_old;
  }

  buf_size = 1024;
  buf = get_buf(&buf_size);
  if (buf == NULL) {
    r = -1;
    goto out_close_new;
  }

  for (offset = 0; offset < dd->size;) {
    len = buf_size;
    if (offset == mask_begin) {
      offset = mask_begin + mask_len;
    } else if (offset + len > mask_begin && offset < mask_begin + mask_len) {
      len = mask_begin - offset;
    }
    if (offset + len > dd->size) {
      len = dd->size - offset;
    }
    DBG(("copy %d @ %d", (int) len, (int) offset));
    if (len > 0) {
      r = sl_FsRead(old_fh, offset, buf, len);
      if (r != len) {
        r = -1;
        goto out_free;
      }
      r = sl_FsWrite(new_fh, offset, buf, len);
      if (r != len) {
        r = -2;
        goto out_free;
      }
      offset += len;
    }
  }

  dd->seq--;
  dd->cidx = new_cidx;
  dd->fh = new_fh;
  new_fh = -1;
  dd->rw = true;
  if (dd->flush_interval_ms > 0) {
    dd->flush_timer_id = mgos_set_timer(
        dd->flush_interval_ms, true /* repeat */, fs_flush_timer_cb, dd);
  }

  r = (fs_write_mount_meta(dd) ? 0 : -1);

  dd->last_write = mg_time();

out_free:
  free(buf);
out_close_new:
  if (new_fh > 0) sl_FsClose(new_fh, NULL, NULL, 0);
out_close_old:
  sl_FsClose(old_fh, NULL, NULL, 0);
  LOG((r == 0 ? LL_DEBUG : LL_ERROR), ("%d", r));
  return r;
}

static bool cc3200_vfs_dev_slfs_container_open(struct mgos_vfs_dev *dev,
                                               const char *opts) {
  int cidx = -1;
  bool ret = false;
  char *cpfx = NULL;
  unsigned int size = 0;
  bool create = false;
  struct dev_data *dd = NULL;
  struct fs_container_info info;
  int flush_interval_ms = MGOS_VFS_DEV_SLFS_CONTAINER_FLUSH_INTERVAL_MS;
  json_scanf(opts, strlen(opts),
             "{prefix: %Q, size: %u, create: %B, flush_interval_ms: %d}", &cpfx,
             &size, &create, &flush_interval_ms);
  if (cpfx == NULL) goto out;
  dd = (struct dev_data *) calloc(1, sizeof(*dd));
  if (dd == NULL) goto out;
  dd->fh = -1;
  dd->flush_timer_id = MGOS_INVALID_TIMER_ID;
  dd->flush_interval_ms = flush_interval_ms;
  cidx = fs_get_active_idx(cpfx, &info);
  if (cidx >= 0) {
    dd->fh = fs_open_container(cpfx, cidx);
    dd->cidx = cidx;
    dd->rw = false;
    dd->seq = info.seq;
    dd->size = info.fs_size;
  } else {
    if (create && size > 0) {
      cidx = 0;
      dd->fh = fs_create_container(cpfx, cidx, size);
      dd->cidx = 0;
      dd->rw = true;
      dd->seq = BOOT_CFG_INITIAL_SEQ;
      dd->size = size;
      dd->last_write = mg_time();
      if (dd->flush_interval_ms > 0) {
        dd->flush_timer_id = mgos_set_timer(
            dd->flush_interval_ms, true /* repeat */, fs_flush_timer_cb, dd);
      }
    }
  }
  if (dd->fh < 0) goto out;
  dd->cpfx = strdup(cpfx);
  dev->dev_data = dd;
  SLIST_INSERT_HEAD(&s_devs, dd, next);
  ret = true;

out:
  LOG(LL_INFO, ("%p %s.%d %lu 0x%llx", dev, cpfx, (dd ? dd->cidx : -1),
                (dd ? dd->size : 0), (dd ? dd->seq : 0)));
  if (!ret) free(dd);
  free(cpfx);
  (void) dev;
  return ret;
}

static bool cc3200_vfs_dev_slfs_container_read(struct mgos_vfs_dev *dev,
                                               size_t offset, size_t size,
                                               void *dst) {
  _i32 r = -1;
  struct dev_data *dd = (struct dev_data *) dev->dev_data;
  LOG(LL_VERBOSE_DEBUG,
      ("%p read %d @ %d, cidx %u # %llu, fh %ld, rw %d", dev, (int) size,
       (int) offset, dd->cidx, dd->seq, dd->fh, dd->rw));
  do {
    if (dd->fh < 0) {
      dd->fh = fs_open_container(dd->cpfx, dd->cidx);
      if (dd->fh < 0) break;
    }
    r = sl_FsRead(dd->fh, offset, dst, size);
    if (r == SL_FS_ERR_INVALID_HANDLE) {
      /*
       * This happens when SimpleLink is reinitialized - all file handles are
       * invalidated. SL has to be reinitialized to e.g. configure WiFi.
       */
      dd->fh = -1;
    }
  } while (dd->fh < 0);
  return (r == size);
}

static bool cc3200_vfs_dev_slfs_container_write(struct mgos_vfs_dev *dev,
                                                size_t offset, size_t size,
                                                const void *src) {
  _i32 r = -1;
  struct dev_data *dd = (struct dev_data *) dev->dev_data;
  LOG(LL_VERBOSE_DEBUG,
      ("%p write %d @ %d, cidx %u # %llu, fh %ld, rw %d", dev, (int) size,
       (int) offset, dd->cidx, dd->seq, dd->fh, dd->rw));
  do {
    if (!dd->rw) {
      if (fs_switch_container(dd, 0, 0) != 0) break;
    }
    r = sl_FsWrite(dd->fh, offset, (unsigned char *) src, size);
    if (r == SL_FS_ERR_INVALID_HANDLE) {
      /*
       * This happens when SimpleLink is reinitialized - all file handles are
       * invalidated. SL has to be reinitialized to e.g. configure WiFi.
       */
      dd->fh = -1;
    }
  } while (dd->fh < 0);
  return (r == size);
}

static bool cc3200_vfs_dev_slfs_container_erase(struct mgos_vfs_dev *dev,
                                                size_t offset, size_t len) {
  struct dev_data *dd = (struct dev_data *) dev->dev_data;
  LOG(LL_VERBOSE_DEBUG, ("%p erase %d @ %d", dev, (int) len, (int) offset));
  return (fs_switch_container(dd, offset, len) == 0);
}

static size_t cc3200_vfs_dev_slfs_container_get_size(struct mgos_vfs_dev *dev) {
  struct dev_data *dd = (struct dev_data *) dev->dev_data;
  return dd->size;
}

static bool cc3200_vfs_dev_slfs_container_close(struct mgos_vfs_dev *dev) {
  struct dev_data *dd = (struct dev_data *) dev->dev_data;
  SLIST_REMOVE(&s_devs, dd, dev_data, next);
  LOG(LL_DEBUG, ("%p fh %ld rw %d", dev, dd->fh, dd->rw));
  fs_close_container(dd);
  free(dd->cpfx);
  free(dd);
  return true;
}

static const struct mgos_vfs_dev_ops cc3200_vfs_dev_slfs_container_ops = {
    .open = cc3200_vfs_dev_slfs_container_open,
    .read = cc3200_vfs_dev_slfs_container_read,
    .write = cc3200_vfs_dev_slfs_container_write,
    .erase = cc3200_vfs_dev_slfs_container_erase,
    .get_size = cc3200_vfs_dev_slfs_container_get_size,
    .close = cc3200_vfs_dev_slfs_container_close,
};

bool cc3200_vfs_dev_slfs_container_register_type(void) {
  return mgos_vfs_dev_register_type(MGOS_DEV_TYPE_SLFS_CONTAINER,
                                    &cc3200_vfs_dev_slfs_container_ops);
}

void cc3200_vfs_dev_slfs_container_delete_container(const char *cpfx,
                                                    int cidx) {
  _i32 ret = -1;
  SlFsFileInfo_t fi;
  _u8 fname[MAX_FS_CONTAINER_FNAME_LEN];
  cc3200_vfs_dev_slfs_container_fname(cpfx, cidx, fname);
  ret = sl_FsGetInfo(fname, cidx, &fi);
  if (ret == 0) {
    LOG(LL_INFO, ("Deleting %s", fname));
    ret = sl_FsDel(fname, 0);
  }
}

void cc3200_vfs_dev_slfs_container_delete_inactive_container(const char *cpfx) {
  struct fs_container_info info;
  int active_idx = fs_get_active_idx(cpfx, &info);
  if (active_idx >= 0) {
    int inactive_idx = active_idx ^ 1;
    cc3200_vfs_dev_slfs_container_delete_container(cpfx, inactive_idx);
  }
}

void cc3200_vfs_dev_slfs_container_flush_all(void) {
  struct dev_data *dd;
  SLIST_FOREACH(dd, &s_devs, next) {
    fs_close_container(dd);
  }
}
