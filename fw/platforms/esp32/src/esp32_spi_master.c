/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_spi.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_attr.h"
#include "esp_intr_alloc.h"
#include "driver/spi_common.h"
#include "soc/gpio_sig_map.h"
#include "soc/spi_reg.h"
#include "soc/spi_struct.h"

#include "common/cs_dbg.h"

#include "fw/src/mgos_hal.h"
#include "fw/src/mgos_sys_config.h"

/* We use AHB addr to avoid https://github.com/espressif/esp-idf/issues/595 */
#define ESP32_SPI2_AHB_BASE 0x60024000
#define ESP32_SPI3_AHB_BASE 0x60025000

struct mgos_spi {
  spi_dev_t *dev;
  spi_host_device_t host;
  unsigned int eff_freq : 28;
  unsigned int mode : 2;
  unsigned int native_pins : 1;
  unsigned int debug : 1;
};

struct mgos_spi *mgos_spi_create(const struct sys_config_spi *cfg) {
  bool claimed = false;
  struct mgos_spi *c = (struct mgos_spi *) calloc(1, sizeof(*c));
  if (c == NULL) goto out_err;

  switch (cfg->unit_no) {
    case 0:
      /* SPI0 is used by flash, do not touch it. */
      goto out_err;
    case 1:
      /* SPI1 shares pins with SPI0, it is in theory possible to use it,
       * but it's tricky, so better not go there. */
      goto out_err;
    case 2:
      c->dev = (spi_dev_t *) ESP32_SPI2_AHB_BASE;
      c->host = HSPI_HOST;
      break;
    case 3:
      c->dev = (spi_dev_t *) ESP32_SPI3_AHB_BASE;
      c->host = VSPI_HOST;
      break;
    default:
      goto out_err;
  }

  if (cfg->sclk_gpio < 0 || (cfg->miso_gpio < 0 && cfg->mosi_gpio < 0)) {
    goto out_err;
  }

  if (!spicommon_periph_claim(c->host)) {
    LOG(LL_ERROR, ("Failed to claim SPI%d (host %d)", cfg->unit_no, c->host));
    goto out_err;
  }
  claimed = true;

  if (!mgos_spi_configure(c, cfg)) {
    goto out_err;
  }

  LOG(LL_INFO,
      ("SPI%d init ok (MISO: %d, MOSI: %d, SCLK: %d; mode %d; freq %u)",
       cfg->unit_no, cfg->miso_gpio, cfg->mosi_gpio, cfg->sclk_gpio, cfg->mode,
       c->eff_freq));
  return c;

out_err:
  if (claimed) spicommon_periph_free(c->host);
  free(c);
  LOG(LL_ERROR, ("Invalid SPI settings"));
  return NULL;
}

bool mgos_spi_configure(struct mgos_spi *c, const struct sys_config_spi *cfg) {
  spi_dev_t *dev = c->dev;

  dev->pin.val = 0;

  if (!mgos_spi_set_mode(c, cfg->mode)) {
    return false;
  }

  /* We don't drive CS. */
  dev->pin.cs0_dis = 1;
  dev->pin.cs1_dis = 1;
  dev->pin.cs2_dis = 1;

  mgos_spi_set_msb_first(c, cfg->msb_first);

  spi_bus_config_t bus_cfg = {
      .mosi_io_num = cfg->mosi_gpio,
      .miso_io_num = cfg->miso_gpio,
      .sclk_io_num = cfg->sclk_gpio,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
  };

  bool is_native;
  if (spicommon_bus_initialize_io(c->host, &bus_cfg, 0 /* dma_chan */,
                                  SPICOMMON_BUSFLAG_MASTER,
                                  &is_native) != ESP_OK) {
    return false;
  }
  c->native_pins = !!is_native;

  if (!mgos_spi_set_freq(c, cfg->freq)) {
    return false;
  }

  if (!mgos_spi_set_msb_first(c, cfg->msb_first)) {
    return false;
  }

  dev->slave.slave_mode = false;

  c->debug = cfg->debug;

  return true;
}

/* See SPI_CLOCK_REG description in the TRM. */
bool mgos_spi_set_freq(struct mgos_spi *c, int freq) {
  spi_dev_t *dev = c->dev;
  int pre, cnt_n;
  if (freq >= APB_CLK_FREQ / 2 && !c->native_pins) {
    /* APB_CLK / 2 and above freqs cannot be routed via GPIO matrix. */
    return false;
  }
  switch (freq) {
    case APB_CLK_FREQ:
      dev->clock.val = 0;
      dev->clock.clk_equ_sysclk = true;
      return true;
    case APB_CLK_FREQ / 2:
      pre = 0;
      cnt_n = 1;
      break;
    case APB_CLK_FREQ / 3:
      pre = 0;
      cnt_n = 2;
      break;
    case APB_CLK_FREQ / 4:
      pre = 0;
      cnt_n = 3;
      break;
    case APB_CLK_FREQ / 10:
      pre = 0;
      cnt_n = 5;
      break;
    default:
      if (freq > APB_CLK_FREQ / 4) {
        /* Cannot be adequately expressed. */
        return false;
      }
      pre = ((APB_CLK_FREQ / 4) / freq) - 1;
      cnt_n = 3;
      break;
  }
  int cnt_h = (cnt_n + 1) / 2 - 1;
  dev->clock.val = 0;
  dev->clock.clkdiv_pre = pre;
  dev->clock.clkcnt_n = cnt_n;
  dev->clock.clkcnt_h = cnt_h;
  dev->clock.clkcnt_l = cnt_n; /* Per TRM, cnt_l = cnt_n */
  c->eff_freq = APB_CLK_FREQ / ((pre + 1) * (cnt_n + 1));
  if (c->debug) {
    LOG(LL_DEBUG, ("freq %d => pre %d cnt_n %d cnt_h %d => eff_freq %u", freq,
                   pre, cnt_n, cnt_h, c->eff_freq));
  }
  return true;
}

bool mgos_spi_set_mode(struct mgos_spi *c, int mode) {
  spi_dev_t *dev = c->dev;
  /* See TRM, section 5.4.1, table 23 */
  int idle_edge, out_edge, delay_mode;
  switch (mode) {
    case 0:
      idle_edge = 0;
      out_edge = 0;
      delay_mode = 2;
      break;
    case 1:
      idle_edge = 0;
      out_edge = 1;
      delay_mode = 1;
      break;
    case 2:
      idle_edge = 1;
      out_edge = 1;
      delay_mode = 1;
      break;
    case 3:
      idle_edge = 1;
      out_edge = 0;
      delay_mode = 2;
      break;
    default:
      return false;
  }
  dev->pin.ck_idle_edge = idle_edge;
  dev->user.ck_out_edge = out_edge;
  dev->ctrl2.miso_delay_mode = delay_mode;
  dev->ctrl2.miso_delay_num = 0;
  dev->ctrl2.mosi_delay_mode = 0;
  dev->ctrl2.mosi_delay_num = 0;
  c->mode = mode;
  return true;
}

bool mgos_spi_set_msb_first(struct mgos_spi *c, bool msb_first) {
  c->dev->ctrl.rd_bit_order = c->dev->ctrl.wr_bit_order = (!msb_first);
  return true;
}

static void esp32_spi_set_tx_data(spi_dev_t *dev, const uint8_t *data,
                                  size_t len) {
  size_t i, wi;
  for (i = 0, wi = 8; i < len; i += 4, wi++) {
    uint32_t w = 0;
    memcpy(&w, data + i, 4);
    dev->data_buf[wi] = w;
  }
}

static void esp32_spi_get_rx_data(spi_dev_t *dev, uint8_t *data, size_t len) {
  size_t i, j, wi;
  for (i = 0, wi = 0; i < len; wi++) {
    uint32_t w = dev->data_buf[wi];
    for (j = 0; j < 4 && i < len; i++, j++) {
      data[i] = (w & 0xff);
      w >>= 8;
    }
  }
}

static void esp32_spi_txn_setup_common(struct mgos_spi *c) {
  spi_dev_t *dev = c->dev;
  bool out_edge = dev->user.ck_out_edge;
  dev->user.val = 0;
  dev->user.ck_out_edge = out_edge;
  dev->user.usr_mosi_highpart = true; /* 0-7 for RX, 8-15 for TX */
  if (c->eff_freq >= APB_CLK_FREQ / 2) {
    /* Add a dummy cycle (user1.usr_dummy_cyclelen is set to 0 below). */
    dev->user.usr_dummy = true;
  }
  dev->user1.val = 0;
  dev->user2.val = 0;
}

bool mgos_spi_txn(struct mgos_spi *c, const void *tx_data, void *rx_data,
                  size_t len) {
  spi_dev_t *dev = c->dev;
  if (c->debug) {
    LOG(LL_DEBUG, ("len %d", (int) len));
  }
  esp32_spi_txn_setup_common(c);
  dev->user.doutdin = true; /* Full duplex mode */

  const uint8_t *txdp = (const uint8_t *) tx_data;
  uint8_t *rxdp = (uint8_t *) rx_data;
  dev->user.usr_mosi = true;
  dev->user.usr_miso = true;
  while (len > 0) {
    size_t dlen = len;
    if (dlen > 16) dlen = 16;
    size_t dbitlen = dlen * 8 - 1;
    dev->mosi_dlen.usr_mosi_dbitlen = dbitlen;
    dev->miso_dlen.usr_miso_dbitlen = dbitlen;
    esp32_spi_set_tx_data(dev, txdp, dlen);
    dev->cmd.usr = true;
    while (dev->cmd.usr) {
    }
    esp32_spi_get_rx_data(dev, rxdp, dlen);
    len -= dlen;
    txdp += dlen;
    rxdp += dlen;
  }

  return true;
}

bool mgos_spi_txn_hd(struct mgos_spi *c, const void *tx_data, size_t tx_len,
                     void *rx_data, size_t rx_len) {
  spi_dev_t *dev = c->dev;
  if (c->debug) {
    LOG(LL_DEBUG, ("tx_len %d rx_len %d", (int) tx_len, (int) rx_len));
  }
  esp32_spi_txn_setup_common(c);
  dev->user.doutdin = false;

  const uint8_t *txdp = (const uint8_t *) tx_data;
  dev->user.usr_mosi = true;
  dev->user.usr_miso = false;
  while (tx_len > 0) {
    size_t dlen = tx_len;
    if (dlen > 16) dlen = 16;
    size_t dbitlen = dlen * 8 - 1;
    dev->mosi_dlen.usr_mosi_dbitlen = dbitlen;
    esp32_spi_set_tx_data(dev, txdp, dlen);
    dev->cmd.usr = true;
    while (dev->cmd.usr) {
    }
    tx_len -= dlen;
    txdp += dlen;
  }

  uint8_t *rxdp = (uint8_t *) rx_data;
  dev->user.usr_mosi = false;
  dev->user.usr_miso = true;
  while (rx_len > 0) {
    size_t dlen = rx_len;
    if (dlen > 16) dlen = 16;
    size_t dbitlen = dlen * 8 - 1;
    dev->miso_dlen.usr_miso_dbitlen = dbitlen;
    dev->cmd.usr = true;
    while (dev->cmd.usr) {
    }
    esp32_spi_get_rx_data(dev, rxdp, dlen);
    rx_len -= dlen;
    rxdp += dlen;
  }

  return true;
}

void mgos_spi_close(struct mgos_spi *c) {
  spicommon_periph_free(c->host);
  free(c);
}
