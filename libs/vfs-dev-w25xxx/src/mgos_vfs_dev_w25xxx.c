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

#include "mgos_vfs_dev_w25xxx.h"

#include "mgos_spi.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "common/cs_dbg.h"
#include "common/platform.h"

#include "frozen.h"

#include "mongoose.h"
#include "mgos_system.h"
#include "mgos_utils.h"
#include "mgos_vfs_dev.h"

#ifndef W25XXX_DEBUG
#define W25XXX_DEBUG 0
#endif
#if W25XXX_DEBUG
#define W25XXX_DEBUG_LEVEL LL_DEBUG
#else
#define W25XXX_DEBUG_LEVEL LL_VERBOSE_DEBUG
#endif

enum w25xxx_op {
  W25XXX_OP_RST = 0xff,
  W25MXX_OP_DIE_SELECT = 0xc2,
  W25XXX_OP_READ_JEDEC_ID = 0x9f,
  W25XXX_OP_READ_REG = 0x05,
  W25XXX_OP_WRITE_REG = 0x01,
  W25XXX_OP_WRITE_ENABLE = 0x06,
  W25XXX_OP_WRITE_DISABLE = 0x04,
  W25XXX_OP_BBM_SWAP_BLOCKS = 0xa1,
  W25XXX_OP_BBM_READ_LUT = 0xa5,
  W25XXX_OP_BBM_READ_LAST_ECC_FAIL_ADDR = 0xa9,
  W25XXX_OP_PROG_DATA_LOAD = 0x02,
  W25XXX_OP_PROG_RAND_DATA_LOAD = 0x84,
  W25XXX_OP_PROG_EXECUTE = 0x10,
  W25XXX_OP_BLOCK_ERASE = 0xd8,
  W25XXX_OP_PAGE_DATA_READ = 0x13,
  W25XXX_OP_READ = 0x03,
};

enum w25xxx_reg {
  W25XXX_REG_PROT = 0xa0, /* Protection register */
  W25XXX_REG_CONF = 0xb0, /* Configuration register */
  W25XXX_REG_STAT = 0xc0, /* Status register */
};

#define W25XXX_REG_CONF_BUF (1 << 3)
#define W25XXX_REG_CONF_ECCE (1 << 4)

#define W25XXX_REG_STAT_BUSY (1 << 0)
#define W25XXX_REG_STAT_WEL (1 << 1)
#define W25XXX_REG_STAT_EFAIL (1 << 2)
#define W25XXX_REG_STAT_PFAIL (1 << 3)
#define W25XXX_REG_STAT_ECC0 (1 << 4)
#define W25XXX_REG_STAT_ECC1 (1 << 5)
#define W25XXX_REG_STAT_LUTF (1 << 6)

#define W25XXX_MAX_NUM_DIES 2
#define W25XXX_BB_LUT_SIZE 20

struct w25xxx_bb_lut_entry {
  uint32_t invalid : 1;
  uint32_t enable : 1;
  uint16_t lba : 10;
  uint16_t pba;
};

struct w25xxx_bb_lut {
  struct w25xxx_bb_lut_entry e[W25XXX_BB_LUT_SIZE];
};

struct w25xxx_dev_data {
  /* SPI settings */
  struct mgos_spi *spi;
  int spi_cs, spi_freq, spi_mode;

  size_t size;
  struct w25xxx_bb_lut bb_lut[W25XXX_MAX_NUM_DIES];
  uint32_t bb_reserve : 8; /* # of blocks to reserve at the end of each die. */
  uint32_t ecc_chk : 1;    /* Check ECC when reading. */
  uint32_t num_dies : 2;   /* Number of dies. Currently max 2 for W25M02xx. */
  uint32_t own_spi : 1;    /* If true, the SPI interface was created by us. */
};

static bool w25xxx_txn(struct w25xxx_dev_data *dd, size_t tx_len,
                       const void *tx_data, int dummy_len, size_t rx_len,
                       void *rx_data) {
  struct mgos_spi_txn txn = {
      .cs = dd->spi_cs, .mode = dd->spi_mode, .freq = dd->spi_freq};
  txn.hd.tx_len = tx_len;
  txn.hd.tx_data = tx_data;
  txn.hd.dummy_len = dummy_len;
  txn.hd.rx_len = rx_len;
  txn.hd.rx_data = rx_data;
  bool res = mgos_spi_run_txn(dd->spi, false /* fd */, &txn);
#if W25XXX_DEBUG
  uint8_t *td = (uint8_t *) tx_data;
  uint8_t op = td[0];
  LOG(LL_DEBUG, ("%02x %02x%02x%02x %u => %d 0x%02x", op,
                 (tx_len > 1 ? td[1] : 0), (tx_len > 2 ? td[2] : 0),
                 (tx_len > 3 ? td[3] : 0), (unsigned int) rx_len, res,
                 (unsigned int) (rx_len > 0 ? *((uint8_t *) rx_data) : 0)));
  if (op == W25XXX_OP_PROG_DATA_LOAD || op == W25XXX_OP_PROG_RAND_DATA_LOAD ||
      tx_len > 4) {
    mg_hexdumpf(stderr, ((uint8_t *) tx_data) + 3, tx_len - 3);
  }
  if (rx_len > 1) mg_hexdumpf(stderr, rx_data, rx_len);
#endif
  return res;
}

static bool w25xxx_op_arg_rx(struct w25xxx_dev_data *dd, enum w25xxx_op op,
                             size_t arg_len, uint32_t arg, int dummy_len,
                             size_t rx_len, void *rx_data) {
  uint8_t buf[5] = {op};
  size_t tx_len;
  switch (arg_len) {
    case 0:
      tx_len = 1;
      break;
    case 1:
      buf[1] = (arg & 0xff);
      tx_len = 2;
      break;
    case 2:
      buf[1] = ((arg >> 8) & 0xff);
      buf[2] = (arg & 0xff);
      tx_len = 3;
      break;
    case 3:
      buf[1] = ((arg >> 16) & 0xff);
      buf[2] = ((arg >> 8) & 0xff);
      buf[3] = (arg & 0xff);
      tx_len = 4;
      break;
    default:
      return false;
  }
  return w25xxx_txn(dd, tx_len, buf, dummy_len, rx_len, rx_data);
}

static bool w25xxx_op_arg(struct w25xxx_dev_data *dd, enum w25xxx_op op,
                          size_t arg_len, uint32_t arg) {
  return w25xxx_op_arg_rx(dd, op, arg_len, arg, 0, 0, NULL);
}

static bool w25xxx_simple_rx_op(struct w25xxx_dev_data *dd, enum w25xxx_op op,
                                int dummy_len, size_t rx_len, void *rx_data) {
  return w25xxx_txn(dd, 1, &op, dummy_len, rx_len, rx_data);
}

static bool w25xxx_simple_op(struct w25xxx_dev_data *dd, enum w25xxx_op op) {
  return w25xxx_txn(dd, 1, &op, 0, 0, NULL);
}

static uint8_t w25xxx_read_reg(struct w25xxx_dev_data *dd,
                               enum w25xxx_reg reg) {
  uint8_t reg_addr = reg, reg_value = 0;
  w25xxx_op_arg_rx(dd, W25XXX_OP_READ_REG, 1, reg_addr, 0, 1, &reg_value);
  return reg_value;
}

static bool w25xxx_write_reg(struct w25xxx_dev_data *dd, enum w25xxx_reg reg,
                             uint8_t value) {
  return w25xxx_op_arg(dd, W25XXX_OP_WRITE_REG, 2,
                       (((uint32_t) reg) << 8 | value));
}

static bool w25mxx_select_die(struct w25xxx_dev_data *dd, uint8_t die_no) {
  if (dd->num_dies == 1) return true;
  if (die_no > dd->num_dies) return false;
  return w25xxx_op_arg(dd, W25MXX_OP_DIE_SELECT, 1, die_no);
}

static bool w25xxx_read_bb_lut(struct w25xxx_dev_data *dd,
                               struct w25xxx_bb_lut *lut, int *num_bb) {
  bool res = false;
  uint8_t tmp[W25XXX_BB_LUT_SIZE * 4];
  if (!w25xxx_op_arg_rx(dd, W25XXX_OP_BBM_READ_LUT, 0, 0, 1, sizeof(tmp),
                        tmp)) {
    goto out;
  }
  for (int i = 0, j = 0; j < (int) ARRAY_SIZE(lut->e); i++, j += 4) {
    struct w25xxx_bb_lut_entry *e = &lut->e[i];
    e->enable = !!(tmp[j] & 0x80);
    e->invalid = !!(tmp[j] & 0x40);
    e->lba = (((uint16_t)(tmp[j] & 3)) << 8) | tmp[j + 1];
    e->pba = (((uint16_t) tmp[j + 2]) << 8) | tmp[j + 3];
    if (num_bb != NULL && e->enable && !e->invalid) (*num_bb)++;
    if (e->enable) {
      LOG(LL_DEBUG,
          ("%d: %d %d %u => %u", i, e->enable, e->invalid, e->lba, e->pba));
    }
  }
  res = true;
out:
  return res;
}

static bool vfs_dev_w25xxx_detect(struct w25xxx_dev_data *dd) {
  bool res = false;
  uint8_t cfg0;
  int num_bb;

  uint8_t jid[3];
  if (!w25xxx_simple_rx_op(dd, W25XXX_OP_READ_JEDEC_ID, 1, sizeof(jid), jid)) {
    LOG(LL_ERROR, ("Failed to read JEDEC ID"));
    goto out;
  }

  if (jid[0] == 0xef && (jid[1] == 0xaa || jid[1] == 0xab) && jid[2] == 0x21) {
    if (jid[1] == 0xaa) { /* W25N01Gxx */
      dd->num_dies = 1;
      dd->size = W25XXX_DIE_SIZE;
    } else { /* W25M02Gxx */
      dd->num_dies = 2;
      dd->size = 2 * W25XXX_DIE_SIZE;
    }
  } else {
    LOG(LL_ERROR,
        ("Invalid chip ID (got %02x %02x %02x)", jid[0], jid[1], jid[2]));
    goto out;
  }

  w25xxx_simple_op(dd, W25XXX_OP_RST);
  mgos_usleep(500);

  cfg0 = w25xxx_read_reg(dd, W25XXX_REG_CONF);

  /* Put all the dies into buffered read mode, unprotect and read BB LUT. */
  num_bb = 0;
  for (uint8_t i = 0; i < dd->num_dies; i++) {
    w25mxx_select_die(dd, i);
    w25xxx_write_reg(dd, W25XXX_REG_PROT, 0);
    w25xxx_write_reg(dd, W25XXX_REG_CONF,
                     cfg0 | W25XXX_REG_CONF_ECCE | W25XXX_REG_CONF_BUF);
    if (!w25xxx_read_bb_lut(dd, &dd->bb_lut[i], &num_bb)) goto out;
  }

  if (dd->bb_reserve > 0) {
    dd->size -= ((size_t) dd->num_dies * dd->bb_reserve * W25XXX_BLOCK_SIZE);
  }

  LOG(LL_INFO,
      ("Found W25%02dGx%s; size %u KiB; bb_reserve %d, num_bb %d",
       (dd->num_dies == 1 ? 1 : 2), (cfg0 & W25XXX_REG_CONF_BUF ? "IG" : "IT"),
       (unsigned int) (dd->size / 1024), dd->bb_reserve, num_bb));

  res = true;

out:
  return res;
}

enum mgos_vfs_dev_err w25xxx_dev_init(struct mgos_vfs_dev *dev,
                                      struct mgos_spi *spi, int spi_cs,
                                      int spi_freq, int spi_mode,
                                      int bb_reserve, bool ecc_chk) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  struct w25xxx_dev_data *dd =
      (struct w25xxx_dev_data *) calloc(1, sizeof(*dd));
  if (dd == NULL) {
    res = MGOS_VFS_DEV_ERR_NOMEM;
    goto out;
  }
  if (spi_freq <= 0) goto out;
  dd->spi = spi;
  dd->spi_cs = spi_cs;
  dd->spi_freq = spi_freq;
  dd->spi_mode = spi_mode;
  dd->bb_reserve = bb_reserve;
  dd->ecc_chk = ecc_chk;
  if (!vfs_dev_w25xxx_detect(dd)) {
    res = MGOS_VFS_DEV_ERR_NXIO;
    goto out;
  }
  dev->dev_data = dd;
  res = MGOS_VFS_DEV_ERR_NONE;
out:
  if (res != 0) free(dd);
  return res;
}

enum mgos_vfs_dev_err vfs_dev_w25xxx_open(struct mgos_vfs_dev *dev,
                                          const char *opts) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_INVAL;
  int cs_num = -1, spi_freq = 0, spi_mode = 0, bb_reserve = 24, ecc_chk = true;
  struct json_token spi_cfg_json = JSON_INVALID_TOKEN;
  struct mgos_spi *spi = NULL;
  json_scanf(opts, strlen(opts),
             "{cs: %d, freq: %d, mode: %d, "
             "bb_reserve: %u, ecc_chk: %B, spi: %T}",
             &cs_num, &spi_freq, &spi_mode, &bb_reserve, &ecc_chk,
             &spi_cfg_json);
  if (spi_cfg_json.ptr != NULL) {
    struct mgos_config_spi spi_cfg = {.enable = true};
    if (!mgos_spi_config_from_json(
            mg_mk_str_n(spi_cfg_json.ptr, spi_cfg_json.len), &spi_cfg)) {
      LOG(LL_ERROR, ("Invalid SPI config"));
      goto out;
    }
    spi = mgos_spi_create(&spi_cfg);
    if (spi == NULL) {
      goto out;
    }
  } else {
    spi = mgos_spi_get_global();
    if (spi == NULL) {
      LOG(LL_INFO, ("SPI is disabled"));
      res = MGOS_VFS_DEV_ERR_NXIO;
      goto out;
    }
  }
  res = w25xxx_dev_init(dev, spi, cs_num, spi_freq, spi_mode, bb_reserve,
                        ecc_chk);
  if (res == 0) {
    struct w25xxx_dev_data *dd = (struct w25xxx_dev_data *) dev->dev_data;
    dd->own_spi = (spi != mgos_spi_get_global());
  }
out:
  return res;
}

static bool w25xxx_map_page_ex(const struct w25xxx_dev_data *dd, size_t off,
                               uint8_t *die_num, uint16_t *page_num,
                               uint16_t *page_off, uint8_t bb_reserve) {
  bool res = false;
  size_t die_size = W25XXX_DIE_SIZE;
  size_t orig_off = off;
  die_size -= ((size_t) bb_reserve) * W25XXX_BLOCK_SIZE;
  *die_num = 0;
  while (off >= die_size) {
    off -= die_size;
    (*die_num)++;
    if (*die_num >= dd->num_dies) goto out;
  }
  *page_num = off / W25XXX_PAGE_SIZE;
  if (page_off != NULL) *page_off = off % W25XXX_PAGE_SIZE;
#if W25XXX_DEBUG
  LOG(LL_DEBUG, ("%u => %u %u %u", (unsigned int) orig_off, *die_num, *page_num,
                 *page_off));
#endif
  res = true;
out:
  (void) orig_off;
  return res;
}

static bool w25xxx_map_page(const struct w25xxx_dev_data *dd, size_t off,
                            uint8_t *die_num, uint16_t *page_num,
                            uint16_t *page_off) {
  return w25xxx_map_page_ex(dd, off, die_num, page_num, page_off,
                            dd->bb_reserve);
}

static enum mgos_vfs_dev_err w25xxx_page_data_read(struct w25xxx_dev_data *dd,
                                                   uint8_t die_num,
                                                   uint16_t page_num) {
  uint8_t st;
  /* Note: note selecting die, assuming already selected. */
  if (!w25xxx_op_arg(dd, W25XXX_OP_PAGE_DATA_READ, 1 + 2, page_num)) {
    return MGOS_VFS_DEV_ERR_IO;
  }
  while ((st = w25xxx_read_reg(dd, W25XXX_REG_STAT)) & W25XXX_REG_STAT_BUSY) {
  }
  if (dd->ecc_chk) {
    st = w25xxx_read_reg(dd, W25XXX_REG_STAT);
    if (st & (W25XXX_REG_STAT_ECC1 | W25XXX_REG_STAT_ECC0)) {
      bool hard = (st & W25XXX_REG_STAT_ECC1);
      LOG((hard ? LL_ERROR : LL_WARN),
          ("%s ECC error @ %u:%u", (hard ? "Hard" : "Soft"), die_num,
           page_num));
      if (hard) return MGOS_VFS_DEV_ERR_CORRUPT;
    }
  }
  return MGOS_VFS_DEV_ERR_NONE;
}

static enum mgos_vfs_dev_err vfs_dev_w25xxx_read(struct mgos_vfs_dev *dev,
                                                 size_t off, size_t len,
                                                 void *dst) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_IO, res2;
  const size_t orig_off = off, orig_len = len;
  struct w25xxx_dev_data *dd = (struct w25xxx_dev_data *) dev->dev_data;
  uint8_t *dp = (uint8_t *) dst;
  uint8_t die_num;
  uint16_t page_num, page_off;
  while (len > 0) {
    if (!w25xxx_map_page(dd, off, &die_num, &page_num, &page_off)) {
      res = MGOS_VFS_DEV_ERR_INVAL;
      goto out;
    }
    size_t rd_len = MIN(len, W25XXX_PAGE_SIZE - page_off);
    if (!w25mxx_select_die(dd, die_num)) goto out;
    if ((res2 = w25xxx_page_data_read(dd, die_num, page_num))) {
      res = res2;
      goto out;
    }
    if (!w25xxx_op_arg_rx(dd, W25XXX_OP_READ, 2, page_off, 1, rd_len, dp)) {
      goto out;
    }
    off += rd_len;
    len -= rd_len;
    dp += rd_len;
  }
  res = MGOS_VFS_DEV_ERR_NONE;
out:
  LOG((res == 0 ? W25XXX_DEBUG_LEVEL : LL_ERROR),
      ("%p read %u @ 0x%x -> %d", dev, (unsigned int) orig_len,
       (unsigned int) orig_off, res));
#if W25XXX_DEBUG
  if (res) mg_hexdumpf(stderr, dst, orig_len);
#endif
  (void) orig_off;
  (void) orig_len;
  return res;
}

static enum mgos_vfs_dev_err vfs_dev_w25xxx_write(struct mgos_vfs_dev *dev,
                                                  size_t off, size_t len,
                                                  const void *src) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_IO, res2;
  const size_t orig_off = off, orig_len = len;
  struct w25xxx_dev_data *dd = (struct w25xxx_dev_data *) dev->dev_data;
  const uint8_t *dp = (const uint8_t *) src;
  uint8_t die_num;
  uint16_t page_num, page_off;
#if W25XXX_DEBUG
  LOG(LL_INFO, ("WR %d @ %x", (int) len, (int) off));
  mg_hexdumpf(stderr, src, len);
#endif
  while (len > 0) {
    if (!w25xxx_map_page(dd, off, &die_num, &page_num, &page_off)) {
      res = MGOS_VFS_DEV_ERR_INVAL;
      goto out;
    }
    uint8_t txn_buf[3 + 128], st;
    size_t wr_len = MIN(len, W25XXX_PAGE_SIZE - page_off);
    if (!w25mxx_select_die(dd, die_num)) goto out;
    /* When modifying part of a page, read it first to ensure correct ECC. */
    if (wr_len != W25XXX_PAGE_SIZE) {
      if ((res2 = w25xxx_page_data_read(dd, die_num, page_num)) != 0) {
        res = res2;
        goto out;
      }
      txn_buf[0] = W25XXX_OP_PROG_RAND_DATA_LOAD;
    } else {
      txn_buf[0] = W25XXX_OP_PROG_DATA_LOAD;
    }
    if (!w25xxx_simple_op(dd, W25XXX_OP_WRITE_ENABLE)) goto out;
    for (size_t txn_off = 0, txn_len = 0; txn_off < wr_len;
         txn_off += txn_len) {
      txn_len = MIN(128, wr_len - txn_off);
      txn_buf[1] = (page_off + txn_off) >> 8;
      txn_buf[2] = (page_off + txn_off) & 0xff;
      memcpy(txn_buf + 3, dp, txn_len);
      if (!w25xxx_txn(dd, 3 + txn_len, txn_buf, 0, 0, NULL)) goto out;
      txn_buf[0] = W25XXX_OP_PROG_RAND_DATA_LOAD;
      dp += txn_len;
    }
    if (!w25xxx_op_arg(dd, W25XXX_OP_PROG_EXECUTE, 1 + 2, page_num)) goto out;
    while ((st = w25xxx_read_reg(dd, W25XXX_REG_STAT)) & W25XXX_REG_STAT_BUSY) {
    }
    if (st & W25XXX_REG_STAT_PFAIL) {
      LOG(LL_ERROR, ("Prog failed, page %u:%u", die_num, page_num));
      /* TODO(rojer): On-the-fly remapping of bad blocks. */
      goto out;
    }
    off += wr_len;
    len -= wr_len;
  }
  res = MGOS_VFS_DEV_ERR_NONE;
out:
  LOG((res == 0 ? W25XXX_DEBUG_LEVEL : LL_ERROR),
      ("%p write %u @ 0x%x -> %d", dev, (unsigned int) orig_len,
       (unsigned int) orig_off, res));
  (void) orig_off;
  (void) orig_len;
  return res;
}

static enum mgos_vfs_dev_err vfs_dev_w25xxx_erase(struct mgos_vfs_dev *dev,
                                                  size_t off, size_t len) {
  enum mgos_vfs_dev_err res = MGOS_VFS_DEV_ERR_IO;
  const size_t orig_off = off, orig_len = len;
  struct w25xxx_dev_data *dd = (struct w25xxx_dev_data *) dev->dev_data;
  if (off % W25XXX_BLOCK_SIZE != 0 || len % W25XXX_BLOCK_SIZE != 0) {
    res = MGOS_VFS_DEV_ERR_INVAL;
    goto out;
  }
  uint8_t die_num;
  uint16_t page_num, page_off;
  while (len > 0) {
    if (!w25xxx_map_page(dd, off, &die_num, &page_num, &page_off)) {
      res = MGOS_VFS_DEV_ERR_INVAL;
      goto out;
    }
    if (!w25mxx_select_die(dd, die_num)) goto out;
    if (!w25xxx_simple_op(dd, W25XXX_OP_WRITE_ENABLE)) goto out;
    if (!w25xxx_op_arg(dd, W25XXX_OP_BLOCK_ERASE, 1 + 2, page_num)) goto out;
    uint8_t st;
    while ((st = w25xxx_read_reg(dd, W25XXX_REG_STAT)) & W25XXX_REG_STAT_BUSY) {
    }
    if (st & W25XXX_REG_STAT_EFAIL) {
      LOG(LL_ERROR, ("Erase failed, page %u:%u", die_num, page_num));
      /* TODO(rojer): On-the-fly remapping of bad blocks. */
      goto out;
    }
    off += W25XXX_BLOCK_SIZE;
    len -= W25XXX_BLOCK_SIZE;
  }
  res = MGOS_VFS_DEV_ERR_NONE;
out:
  LOG((res == 0 ? W25XXX_DEBUG_LEVEL : LL_ERROR),
      ("%p erase %u @ 0x%x (pg %u blk %u) -> %d", dev, (unsigned int) orig_len,
       (unsigned int) orig_off, (unsigned int) (orig_off / W25XXX_PAGE_SIZE),
       (unsigned int) (orig_off / W25XXX_BLOCK_SIZE), res));
  (void) orig_off;
  (void) orig_len;
  return res;
}

static size_t vfs_dev_w25xxx_get_size(struct mgos_vfs_dev *dev) {
  struct w25xxx_dev_data *dd = (struct w25xxx_dev_data *) dev->dev_data;
  return dd->size;
}

static enum mgos_vfs_dev_err vfs_dev_w25xxx_close(struct mgos_vfs_dev *dev) {
  struct w25xxx_dev_data *dd = (struct w25xxx_dev_data *) dev->dev_data;
  if (dd->own_spi) mgos_spi_close(dd->spi);
  return MGOS_VFS_DEV_ERR_NONE;
}

static enum mgos_vfs_dev_err vfs_dev_w25xxx_get_erase_sizes(
    struct mgos_vfs_dev *dev, size_t sizes[MGOS_VFS_DEV_NUM_ERASE_SIZES]) {
  sizes[0] = W25XXX_BLOCK_SIZE;
  (void) dev;
  return MGOS_VFS_DEV_ERR_NONE;
}

bool w25xxx_remap_block(struct mgos_vfs_dev *dev, size_t bad_off,
                        size_t good_off) {
  bool res = false;
  struct w25xxx_dev_data *dd = (struct w25xxx_dev_data *) dev->dev_data;
  uint8_t bdn;
  uint16_t bpn, lba, pba;
  struct w25xxx_bb_lut *lut;
  if (!w25xxx_map_page_ex(dd, bad_off, &bdn, &bpn, NULL, false)) {
    goto out;
  }
  uint8_t gdn;
  uint16_t gpn;
  if (!w25xxx_map_page_ex(dd, good_off, &gdn, &gpn, NULL, false)) {
    goto out;
  }
  if (bdn != gdn) goto out; /* Cannot remap between different dies. */
  lba = bpn >> 6, pba = gpn >> 6;
  /*
   * The datasheet says:
   *   Registering the same address in multiple PBAs is prohibited.
   *   It may cause unexpected behavior.
   */
  lut = &dd->bb_lut[bdn];
  for (int i = 0; i < (int) ARRAY_SIZE(lut->e); i++) {
    if (lut->e[i].lba == lba) {
      LOG(LL_ERROR, ("Die %u: dup BB LUT entry for LBA %u", bdn, lba));
      goto out;
    }
  }
  if (!w25mxx_select_die(dd, bdn)) goto out;
  if (w25xxx_read_reg(dd, W25XXX_REG_STAT) & W25XXX_REG_STAT_LUTF) {
    LOG(LL_ERROR, ("Die %u: BB LUT full", bdn));
    goto out;
  }
  {
    uint8_t tx_data[5] = {
        W25XXX_OP_BBM_SWAP_BLOCKS, (lba >> 8) & 0xff, (lba & 0xff),
        (pba >> 8) & 0xff, (pba & 0xff),
    };
    if (!w25xxx_simple_op(dd, W25XXX_OP_WRITE_ENABLE)) goto out;
    if (!w25xxx_txn(dd, sizeof(tx_data), tx_data, 0, 0, NULL)) goto out;
  }
  LOG(LL_INFO, ("Remap die %u block %u => %u", bdn, lba, pba));
  if (!w25xxx_read_bb_lut(dd, lut, NULL)) goto out;
  res = true;
out:

  return res;
}

const struct mgos_vfs_dev_ops mgos_vfs_dev_w25xxx_ops = {
    .open = vfs_dev_w25xxx_open,
    .read = vfs_dev_w25xxx_read,
    .write = vfs_dev_w25xxx_write,
    .erase = vfs_dev_w25xxx_erase,
    .get_size = vfs_dev_w25xxx_get_size,
    .close = vfs_dev_w25xxx_close,
    .get_erase_sizes = vfs_dev_w25xxx_get_erase_sizes,
};

bool mgos_vfs_dev_w25xxx_init(void) {
  return mgos_vfs_dev_register_type(MGOS_VFS_DEV_TYPE_W25XXX,
                                    &mgos_vfs_dev_w25xxx_ops);
}
