/* Low-level FS interface, SPIFFS-on-FailFS container. */

#include "cc3200_fs_spiffs_container.h"

#include <malloc.h>

#include "hw_types.h"
#include "simplelink.h"
#include "device.h"
#include "fs.h"

#include "v7.h"
#include "config.h"
#include "spiffs_nucleus.h"

#define INITIAL_SEQ (~(0ULL) - 1ULL)

/* TI recommends rounding to nearest multiple of 4K - 512 bytes.
 * However, experiments have shown that you need to leave 1024 bytes at the end
 * otherwise additional 4K is allocated (compare AllocatedLen vs FileLen). */
#define FS_CONTAINER_SIZE(fs_size) (((((fs_size) >> 12) + 1) << 12) - 1024)

struct fs_info {
  _u64 seq;
  _u32 fs_size;
  _u32 fs_block_size;
  _u32 fs_page_size;
} info;

union fs_meta {
  struct fs_info info;
  _u8 padding[64];
};

struct mount_info s_fsm;

static const _u8 *container_fname(int cidx) {
  return (const _u8 *) (cidx ? "1.fs" : "0.fs");
}

static _i32 fs_create_container(int cidx, _u32 fs_size) {
  _i32 fh = -1;
  const _u8 *fname = container_fname(cidx);
  _u32 fsize = FS_CONTAINER_SIZE(fs_size);
  int r = sl_FsDel(fname, 0);
  dprintf(("del %s -> %d\n", fname, r));
  r = sl_FsOpen(fname, FS_MODE_OPEN_CREATE(fsize, 0), NULL, &fh);
  dprintf(("open %s %d -> %d %d\n", fname, (int) fsize, (int) r, (int) fh));
  if (r != 0) return r;
  return fh;
}

static _i32 fs_write_meta(struct mount_info *m) {
  int r;
  _u32 offset;
  union fs_meta meta;
  memset(&meta, 0xff, sizeof(meta));
  meta.info.seq = m->seq;
  meta.info.fs_size = m->fs.cfg.phys_size;
  meta.info.fs_block_size = m->fs.cfg.log_block_size;
  meta.info.fs_page_size = m->fs.cfg.log_page_size;

  offset = FS_CONTAINER_SIZE(meta.info.fs_size) - sizeof(meta);
  r = sl_FsWrite(m->fh, offset, (_u8 *) &meta, sizeof(meta));
  dprintf(("write meta %llu @ %d: %d\n", m->seq, (int) offset, (int) r));
  if (r == sizeof(meta)) r = 0;
  return r;
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
  dprintf(("switch %d -> %d\n", m->cidx, new_cidx));
  if (old_fh < 0) {
    r = sl_FsOpen(container_fname(m->cidx), FS_MODE_OPEN_READ, NULL, &old_fh);
    dprintf(("fopen %d\n", r));
    if (r < 0) {
      r = SPIFFS_ERR_NOT_READABLE;
      goto out_close_old;
    }
  }
  new_fh = fs_create_container(new_cidx, m->fs.cfg.phys_size);
  if (new_fh < 0) {
    r = new_fh;
    goto out_close_old;
  }

  buf_size = 8192;
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
    dprintf(("copy %d @ %d\n", (int) len, (int) offset));
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

  r = fs_write_meta(m);

out_free:
  free(buf);
out_close_new:
  if (new_fh > 0) sl_FsClose(new_fh, NULL, NULL, 0);
out_close_old:
  sl_FsClose(old_fh, NULL, NULL, 0);
  dprintf(("switch: %d\n", r));
  return r;
}

void fs_close_container(struct mount_info *m) {
  if (!m->valid || !m->rw) return;
  dprintf(("closing fh %d\n", (int) m->fh));
  sl_FsClose(m->fh, NULL, NULL, 0);
  m->fh = -1;
  m->rw = 0;
}

static s32_t failfs_read(u32_t addr, u32_t size, u8_t *dst) {
  struct mount_info *m = &s_fsm;
  _i32 r;
  dprintf(("failfs_read %d @ %d, cidx %u # %llu, fh %d, valid %d, rw %d\n",
           (int) size, (int) addr, m->cidx, m->seq, (int) m->fh, m->valid,
           m->rw));
  if (!m->valid) return SPIFFS_ERR_NOT_READABLE;
  if (m->fh < 0) {
    r = sl_FsOpen(container_fname(m->cidx), FS_MODE_OPEN_READ, NULL, &m->fh);
    dprintf(("fopen %d\n", (int) r));
    if (r < 0) return SPIFFS_ERR_NOT_READABLE;
  }
  r = sl_FsRead(m->fh, addr, dst, size);
  dprintf(("read %d\n", (int) r));
  return (r == size) ? SPIFFS_OK : SPIFFS_ERR_NOT_READABLE;
}

static s32_t failfs_write(u32_t addr, u32_t size, u8_t *src) {
  struct mount_info *m = &s_fsm;
  _i32 r;
  dprintf(("failfs_write %d @ %d, cidx %d # %llu, fh %d, valid %d, rw %d\n",
           (int) size, (int) addr, m->cidx, m->seq, (int) m->fh, m->valid,
           m->rw));
  if (!m->valid) return SPIFFS_ERR_NOT_WRITABLE;
  if (!m->rw) {
    /* Remount rw. */
    if (fs_switch_container(m, 0, 0) != 0) return SPIFFS_ERR_NOT_WRITABLE;
  }
  r = sl_FsWrite(m->fh, addr, src, size);
  dprintf(("write %d\n", (int) r));
  return (r == size) ? SPIFFS_OK : SPIFFS_ERR_NOT_WRITABLE;
}

static s32_t failfs_erase(u32_t addr, u32_t size) {
  struct mount_info *m = &s_fsm;
  dprintf(("failfs_erase %d @ %d\n", (int) size, (int) addr));
  if (m->formatting) {
    /* During formatting, the file is brand new and has just been erased. */
    return SPIFFS_OK;
  }
  return fs_switch_container(m, addr, size) == 0 ? SPIFFS_OK
                                                 : SPIFFS_ERR_ERASE_FAIL;
}

static int fs_get_info(int cidx, struct fs_info *info) {
  union fs_meta meta;
  SlFsFileInfo_t fi;
  _i32 fh;
  _u32 offset;
  _i32 r = sl_FsGetInfo(container_fname(cidx), 0, &fi);
  dprintf(("finfo %s %d %d %d\n", container_fname(cidx), (int) r,
           (int) fi.FileLen, (int) fi.AllocatedLen));
  if (r != 0) return r;
  if (fi.AllocatedLen < sizeof(meta)) return -200;
  r = sl_FsOpen(container_fname(cidx), FS_MODE_OPEN_READ, NULL, &fh);
  dprintf(("fopen %s %d\n", container_fname(cidx), (int) r));
  if (r != 0) return r;

  offset = fi.FileLen - sizeof(meta);
  r = sl_FsRead(fh, offset, (_u8 *) &meta, sizeof(meta));
  dprintf(("read meta @ %d: %d\n", (int) offset, (int) r));

  if (r != sizeof(meta)) {
    r = -201;
    goto out_close;
  }
  if (meta.info.seq > INITIAL_SEQ || meta.info.fs_size < 0) {
    r = -202;
    goto out_close;
  }

  memcpy(info, &meta.info, sizeof(*info));
  dprintf(("found fs: %llu %d %d %d\n", info->seq, (int) info->fs_size,
           (int) info->fs_block_size, (int) info->fs_page_size));
  r = 0;

out_close:
  sl_FsClose(fh, NULL, NULL, 0);
  return r;
}

static _i32 fs_mount_spiffs(struct mount_info *m, _u32 fs_size, _u32 block_size,
                            _u32 page_size) {
  int r;
  spiffs_config cfg;
  cfg.hal_read_f = failfs_read;
  cfg.hal_write_f = failfs_write;
  cfg.hal_erase_f = failfs_erase;
  cfg.phys_size = fs_size;
  cfg.phys_addr = 0;
  cfg.phys_erase_block = block_size;
  cfg.log_block_size = block_size;
  cfg.log_page_size = page_size;
  m->work = calloc(2, page_size);
  m->fds_size = MAX_OPEN_SPIFFS_FILES * sizeof(spiffs_fd);
  m->fds = calloc(1, m->fds_size);
  r = SPIFFS_mount(&m->fs, &cfg, m->work, m->fds, m->fds_size, NULL, 0, NULL);
  if (r != SPIFFS_OK) {
    free(m->work);
    free(m->fds);
    m->work = m->fds = NULL;
  }
  return r;
}

static int fs_format(int cidx) {
  s32_t r;
  struct mount_info *m = &s_fsm;
  _u32 fsc_size = FS_CONTAINER_SIZE(FS_SIZE);
  dprintf(("formatting %d s=%u, cs=%d\n", cidx, FS_SIZE, (int) fsc_size));

  m->cidx = cidx;
  r = fs_create_container(cidx, FS_SIZE);
  if (r < 0) goto out;
  m->fh = r;
  m->valid = m->rw = m->formatting = 1;
  /* Touch a byte at the end to open a "hole". */
  r = sl_FsWrite(m->fh, fsc_size - 1, (_u8 *) "\xff", 1);
  dprintf(("write 1 @ %d %d\n", (int) (fsc_size - 1), (int) r));
  if (r != 1) goto out_close;

  /* There must be a mount attempt before format. It'll fail and that's ok. */
  r = fs_mount_spiffs(m, FS_SIZE, FS_BLOCK_SIZE, FS_PAGE_SIZE);
  dprintf(("mount: %d %d\n", (int) r, (int) SPIFFS_errno(&m->fs)));
  r = SPIFFS_format(&m->fs);
  dprintf(("format: %d\n", (int) r));
  if (r != SPIFFS_OK) goto out_close;

  m->seq = INITIAL_SEQ;
  r = fs_write_meta(m);

out_close:
  sl_FsClose(m->fh, NULL, NULL, 0);
out:
  m->fh = -1;
  m->valid = m->formatting = m->rw = 0;
  return r;
}

static int fs_mount(int cidx, struct mount_info *m) {
  int r;
  struct fs_info fsi;
  memset(m, 0, sizeof(*m));
  m->fh = -1;
  dprintf(("mounting %d\n", cidx));
  m->cidx = cidx;
  r = fs_get_info(cidx, &fsi);
  if (r != 0) return r;
  m->seq = fsi.seq;
  m->valid = 1;
  r = fs_mount_spiffs(m, FS_SIZE, FS_BLOCK_SIZE, FS_PAGE_SIZE);
  dprintf(("mount %d: %d %d\n", cidx, (int) r, (int) SPIFFS_errno(&m->fs)));
  return r;
}

int init_fs(struct v7 *v7) {
  struct fs_info fs0, fs1;
  int r, r0, r1;

  r0 = fs_get_info(0, &fs0);
  r1 = fs_get_info(1, &fs1);

  dprintf(("r0 = %d, r1 = %d\n", r0, r1));

  if (r0 == 0 && r1 == 0) {
    if (fs0.seq < fs1.seq) {
      r1 = -1;
    } else {
      r0 = -1;
    }
  }
  if (r0 == 0) {
    r = fs_mount(0, &s_fsm);
  } else if (r1 == 0) {
    r = fs_mount(1, &s_fsm);
  } else {
    r = fs_format(0);
    if (r != 0) return r;
    r = fs_mount(0, &s_fsm);
  }

  return r;
}
