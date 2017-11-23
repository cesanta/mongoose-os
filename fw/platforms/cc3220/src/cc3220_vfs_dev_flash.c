/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "cc3220_vfs_dev_flash.h"

#include "inc/hw_types.h"
#include "inc/hw_flash_ctrl.h"
#include "inc/hw_memmap.h"
#include "driverlib/flash.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"

#include "common/cs_dbg.h"
#include "common/platform.h"

#include "frozen/frozen.h"
#include "mongoose/mongoose.h"

#include "mgos_hal.h"
#include "mgos_utils.h"
#include "mgos_vfs_dev.h"

#define MGOS_DEV_CC3220_FLASH_DEFAULT_OFFSET 0x80000
#define CC3220_FLASH_SIZE 0x100000
#define CC3220_FLASH_SECTOR_SIZE 0x800
#define CC3220_FLASH_WRITE_ALIGN 4
#define CC3220_FLASH_MMAP_BASE 0x01000000

struct dev_data {
  int offset;
  int size;
};

static bool cc3220_vfs_dev_flash_write(struct mgos_vfs_dev *dev, size_t offset,
                                       size_t size, const void *src);
static bool cc3220_vfs_dev_flash_erase(struct mgos_vfs_dev *dev, size_t offset,
                                       size_t size);

static bool cc3220_vfs_dev_flash_open(struct mgos_vfs_dev *dev,
                                      const char *opts) {
  int fh = 1;
  bool ret = false;
  unsigned char *image = NULL;
  unsigned char *buf = NULL;
  struct dev_data *dd = (struct dev_data *) calloc(1, sizeof(*dd));
  if (dd == NULL) goto out;
  dd->offset = MGOS_DEV_CC3220_FLASH_DEFAULT_OFFSET;
  dd->size = -1;
  json_scanf(opts, strlen(opts), "{offset: %d, size: %d, image: %Q}",
             &dd->offset, &dd->size, &image);
  if (dd->size < 0) dd->size = CC3220_FLASH_SIZE - dd->offset;
  if (dd->offset > CC3220_FLASH_SIZE - CC3220_FLASH_SECTOR_SIZE ||
      dd->offset % CC3220_FLASH_SECTOR_SIZE != 0) {
    LOG(LL_ERROR,
        ("Invalid offset: 0x%x, must be between 0 and 0x%x and aligned to 0x%x",
         dd->offset, CC3220_FLASH_SIZE - CC3220_FLASH_SECTOR_SIZE,
         CC3220_FLASH_SECTOR_SIZE));
    goto out;
  }
  if (dd->offset + dd->size > CC3220_FLASH_SIZE ||
      dd->size < CC3220_FLASH_SECTOR_SIZE) {
    LOG(LL_ERROR, ("Invalid size %d for offset 0x%x: must be at least %d and "
                   "not exceed %d",
                   dd->size, dd->offset, CC3220_FLASH_SECTOR_SIZE,
                   CC3220_FLASH_SIZE - dd->offset));
    goto out;
  }
  dev->dev_data = dd;
  if (image != NULL) {
    int dup = 0;
    SlFsFileInfo_t fi;
    if (sl_FsGetInfo(image, 0, &fi) < 0) {
      /* No image - that's fine. */
      goto out_ok;
    }
    if (fi.Len != dd->size) {
      LOG(LL_ERROR, ("Wrong device image (%s) size: expected %d, found %d",
                     image, dd->size, (int) fi.Len));
      goto out;
    }
    fh = slfs_open(image, SL_FS_READ);
    if (fh < 0) {
      LOG(LL_ERROR, ("Failed to open %s: %d", image, fh));
      goto out;
    }
    LOG(LL_INFO, ("Found device image %s, copying to 0x%x", image, dd->offset));
    /* Since we already found size to be equal to expected, we can assume it's
     * mod sector size. */
    buf = (unsigned char *) calloc(1, CC3220_FLASH_SECTOR_SIZE);
    for (int offset = 0; offset < dd->size;
         offset += CC3220_FLASH_SECTOR_SIZE) {
      int nr = sl_FsRead(fh, offset, (unsigned char *) buf,
                         CC3220_FLASH_SECTOR_SIZE);
      if (nr != CC3220_FLASH_SECTOR_SIZE) {
        LOG(LL_ERROR, ("Failed to read %s: %d @ %d: %d", image,
                       CC3220_FLASH_SECTOR_SIZE, offset, nr));
        goto out;
      }
      uintptr_t mem_offset = (CC3220_FLASH_MMAP_BASE + dd->offset + offset);
      if (memcmp(buf, (const void *) mem_offset, CC3220_FLASH_SECTOR_SIZE) ==
          0) {
        dup += nr;
        continue;
      }
      if (!cc3220_vfs_dev_flash_erase(dev, offset, CC3220_FLASH_SECTOR_SIZE)) {
        goto out;
      }
      if (!cc3220_vfs_dev_flash_write(dev, offset, CC3220_FLASH_SECTOR_SIZE,
                                      buf)) {
        goto out;
      }
      mgos_wdt_feed();
    }
    sl_FsClose(fh, NULL, NULL, 0);
    /* Wipe the source image: overwrite with zeroes. */
    fh = slfs_open(image, SL_FS_WRITE);
    if (fh >= 0) {
      memset(buf, 0, CC3220_FLASH_SECTOR_SIZE);
      for (int offset = 0; offset < dd->size;
           offset += CC3220_FLASH_SECTOR_SIZE) {
        int nw = sl_FsWrite(fh, offset, buf, CC3220_FLASH_SECTOR_SIZE);
        if (nw != CC3220_FLASH_SECTOR_SIZE) {
          LOG(LL_INFO, ("Failed to zero source image @ %d: %d", offset, nw));
          break;
        }
        mgos_wdt_feed();
      }
      sl_FsClose(fh, NULL, NULL, 0);
    }
    fh = -1;
    int r = sl_FsDel(image, 0);
    if (r < 0) {
      LOG(LL_INFO, ("Failed to delete image after copying: %d", r));
      goto out;
    }
    LOG(LL_INFO, ("Copied %d bytes (%d dup)", dd->size, dup));
  }

out_ok:
  ret = true;

out:
  if (!ret) free(dd);
  free(image);
  free(buf);
  if (fh >= 0) sl_FsClose(fh, NULL, NULL, 0);
  return ret;
}

static bool cc3220_vfs_dev_flash_read(struct mgos_vfs_dev *dev, size_t offset,
                                      size_t size, void *dst) {
  bool ret = false;
  struct dev_data *dd = (struct dev_data *) dev->dev_data;
  if (offset > dd->size || offset + size > dd->size) goto out;
  memcpy(dst, (const uint8_t *) (CC3220_FLASH_MMAP_BASE + dd->offset + offset),
         size);
  ret = true;
out:
  LOG((ret ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("%p read %d @ %d => %d", dev, (int) size, (int) offset, ret));
  return ret;
}

static bool flash_program(struct dev_data *dd, size_t offset, size_t size,
                          const void *src) {
  LOG(LL_VERBOSE_DEBUG, ("prog %d @ %d", (int) size, (int) offset));
  if (MAP_FlashProgram((unsigned long *) src,
                       CC3220_FLASH_MMAP_BASE + dd->offset + offset,
                       size) != 0) {
    LOG(LL_ERROR, ("Failed to write %d @ 0x%x st %x", (int) size, offset,
                   HWREG(FLASH_CONTROL_BASE + FLASH_CTRL_O_FCRIS)));
    return false;
  }
  return true;
}

static bool cc3220_vfs_dev_flash_write(struct mgos_vfs_dev *dev, size_t offset,
                                       size_t size, const void *src) {
  bool ret = false;
  struct dev_data *dd = (struct dev_data *) dev->dev_data;
  const uint8_t *sp = (const uint8_t *) src;
  size_t orig_offset = offset, orig_size = size;
  size_t aligned_size = 0;
  if (offset > dd->size || offset + size > dd->size) goto out;

  /* Align write offset, if necessary. */
  if (offset % CC3220_FLASH_WRITE_ALIGN != 0) {
    uint8_t align_buf[CC3220_FLASH_WRITE_ALIGN];
    size_t rem = offset % CC3220_FLASH_WRITE_ALIGN;
    size_t rem_size = CC3220_FLASH_WRITE_ALIGN - rem;
    size_t aligned_offset = offset - rem;
    if (rem_size > size) rem_size = size;
    if (!cc3220_vfs_dev_flash_read(dev, aligned_offset,
                                   CC3220_FLASH_WRITE_ALIGN, align_buf)) {
      goto out;
    }
    LOG(LL_VERBOSE_DEBUG, ("%p rem %d rem_size %d", dev, rem, rem_size));
    memcpy(align_buf + rem, sp, rem_size);
    if (!flash_program(dd, aligned_offset, CC3220_FLASH_WRITE_ALIGN,
                       align_buf)) {
      goto out;
    }
    offset += rem_size;
    size -= rem_size;
    sp += rem_size;
  }
  /* Write the aligned portion. */
  aligned_size = size & ~3;
  if (aligned_size > 0) {
    if (!flash_program(dd, offset, aligned_size, sp)) {
      goto out;
    }
    offset += aligned_size;
    size -= aligned_size;
    sp += aligned_size;
  }
  /* Align and write the remainder. */
  if (size > 0) {
    uint8_t align_buf[CC3220_FLASH_WRITE_ALIGN];
    assert(size < CC3220_FLASH_WRITE_ALIGN);
    assert(offset % CC3220_FLASH_WRITE_ALIGN == 0);
    if (!cc3220_vfs_dev_flash_read(dev, offset, CC3220_FLASH_WRITE_ALIGN,
                                   align_buf)) {
      goto out;
    }
    memcpy(align_buf, sp, size);
    if (!flash_program(dd, offset, CC3220_FLASH_WRITE_ALIGN, align_buf)) {
      goto out;
    }
  }
  ret = true;
out:
  LOG((ret ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("%p write %d @ %d => %d", dev, (int) orig_size, (int) orig_offset, ret));
  return ret;
}

static bool cc3220_vfs_dev_flash_erase(struct mgos_vfs_dev *dev, size_t offset,
                                       size_t size) {
  bool ret = false;
  struct dev_data *dd = (struct dev_data *) dev->dev_data;
  size_t orig_offset = offset, orig_size = size;
  if (offset > dd->size || offset + size > dd->size ||
      (offset % CC3220_FLASH_SECTOR_SIZE != 0) ||
      (size % CC3220_FLASH_SECTOR_SIZE != 0)) {
    goto out;
  }
  while (size > 0) {
    if (MAP_FlashErase(CC3220_FLASH_MMAP_BASE + dd->offset + offset) != 0) {
      goto out;
    }
    offset += CC3220_FLASH_SECTOR_SIZE;
    size -= CC3220_FLASH_SECTOR_SIZE;
  }
  ret = true;
out:
  LOG((ret ? LL_VERBOSE_DEBUG : LL_ERROR),
      ("%p erase %d @ %d => %d", dev, (int) orig_size, (int) orig_offset, ret));
  return ret;
}

static size_t cc3220_vfs_dev_flash_get_size(struct mgos_vfs_dev *dev) {
  struct dev_data *dd = (struct dev_data *) dev->dev_data;
  return dd->size;
}

static bool cc3220_vfs_dev_flash_close(struct mgos_vfs_dev *dev) {
  struct dev_data *dd = (struct dev_data *) dev->dev_data;
  free(dd);
  return true;
}

static const struct mgos_vfs_dev_ops cc3220_vfs_dev_flash_ops = {
    .open = cc3220_vfs_dev_flash_open,
    .read = cc3220_vfs_dev_flash_read,
    .write = cc3220_vfs_dev_flash_write,
    .erase = cc3220_vfs_dev_flash_erase,
    .get_size = cc3220_vfs_dev_flash_get_size,
    .close = cc3220_vfs_dev_flash_close,
};

bool cc3220_vfs_dev_flash_register_type(void) {
  return mgos_vfs_dev_register_type(MGOS_DEV_TYPE_FLASH,
                                    &cc3220_vfs_dev_flash_ops);
}
