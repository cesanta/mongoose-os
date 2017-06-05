/*
 * Copyright (c) 2014-2017 Cesanta Software Limited
 * All rights reserved
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "esp_attr.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "soc/gpio_sig_map.h"
#include "soc/i2c_reg.h"
#include "soc/i2c_struct.h"

#include "fw/src/mgos_i2c.h"
#include "fw/src/mgos_sys_config.h"

#include "common/cs_dbg.h"

#define I2C_COMMAND_OP_RSTART (0 << 11)
#define I2C_COMMAND_OP_WRITE (1 << 11)
#define I2C_COMMAND_OP_READ (2 << 11)
#define I2C_COMMAND_OP_STOP (3 << 11)
#define I2C_COMMAND_OP_END (4 << 11)
#define I2C_COMMAND_ACK_VALUE (1 << 10)
#define I2C_COMMAND_ACK_EXP (1 << 9)
#define I2C_COMMAND_ACK_CHECK (1 << 8)

#define I2C_TX_FIFO_LEN 32
#define I2C_RX_FIFO_LEN 32

/* We use AHB addr to avoid https://github.com/espressif/esp-idf/issues/595 */
#define ESP32_I2C0_AHB_BASE 0x60013000
#define ESP32_I2C1_AHB_BASE 0x60027000

struct mgos_i2c {
  i2c_dev_t *dev;
  int freq;
  bool debug;
};

#define I2C_DONE_INTS (I2C_TRANS_COMPLETE_INT_RAW | I2C_END_DETECT_INT_RAW)
#define I2C_ERROR_INTS                                                         \
  (I2C_ACK_ERR_INT_RAW | I2C_TIME_OUT_INT_RAW | I2C_ARBITRATION_LOST_INT_RAW | \
   I2C_RXFIFO_OVF_INT_RAW | I2C_TIME_OUT_INT_RAW)

static void esp32_i2c_write_addr(i2c_dev_t *dev, uint16_t addr, bool read,
                                 int *ci) {
  uint8_t addr_len = 0;
  dev->fifo_conf.tx_fifo_rst = true;
  dev->fifo_conf.tx_fifo_rst = false;
  dev->command[(*ci)++].val = I2C_COMMAND_OP_RSTART;
  if (addr < 0x7f) {
    dev->fifo_data.data = ((addr & 0x7f) << 1) | read;
    addr_len = 1;
  } else {
    /* https://www.i2c-bus.org/addressing/10-bit-addressing/ */
    dev->fifo_data.data = (0xf0 | ((addr >> 8) & 3) | read);
    dev->fifo_data.data = (addr & 0xff);
    addr_len = 2;
  }
  dev->command[(*ci)++].val =
      (I2C_COMMAND_OP_WRITE | I2C_COMMAND_ACK_CHECK) | addr_len;
}

static bool esp32_i2c_exec(struct mgos_i2c *c, int num_cmds, int exp_tx,
                           int exp_rx) {
  i2c_dev_t *dev = c->dev;
  int loops = 0;
  bool ok = false;
  uint32_t ints = dev->int_raw.val;
#if 0
  if (c->debug) {
    LOG(LL_DEBUG,
        ("  begin: tx_fifo %u rx_fifo %u ints 0x%08x",
         dev->status_reg.tx_fifo_cnt, dev->status_reg.rx_fifo_cnt, ints));
    for (int ci = 0; ci < num_cmds; ci++) {
      LOG(LL_DEBUG,
          ("    %08x %d", dev->command[ci].val, dev->command[ci].op_code));
    }
  }
#endif
  dev->int_clr.val = (I2C_DONE_INTS | I2C_ERROR_INTS);
  uint32_t ints_before = dev->int_raw.val;
  dev->ctr.trans_start = true;
  uint32_t old_fifo_cnt =
      dev->status_reg.tx_fifo_cnt + dev->status_reg.rx_fifo_cnt;
  uint32_t max_loops = 10000;
  do {
    loops++;
    ints = dev->int_raw.val;
    uint32_t fifo_cnt =
        dev->status_reg.tx_fifo_cnt + dev->status_reg.rx_fifo_cnt;
    if (fifo_cnt != old_fifo_cnt) {
      max_loops += 10000;
      old_fifo_cnt = fifo_cnt;
    }
  } while (!(ints & (I2C_DONE_INTS | I2C_ERROR_INTS)) && loops < max_loops);
  ok = (ints & I2C_DONE_INTS) && !(ints & I2C_ERROR_INTS);
  if (ok) {
    if ((exp_tx >= 0 && dev->status_reg.tx_fifo_cnt != exp_tx) ||
        (exp_rx >= 0 && dev->status_reg.rx_fifo_cnt != exp_rx)) {
      ok = false;
    }
  }
  if (c->debug) {
    LOG(LL_DEBUG,
        ("  %d loops, ok? %d, ints 0x%08x -> 0x%08x, tx_fifo %u/%d rx_fifo "
         "%u/%d, "
         "status 0x%08x",
         loops, ok, ints_before, ints, dev->status_reg.tx_fifo_cnt, exp_tx,
         dev->status_reg.rx_fifo_cnt, exp_rx, dev->status_reg.val));
    for (int ci = 0; !ok && ci < num_cmds; ci++) {
      LOG(LL_DEBUG,
          ("    %08x %d", dev->command[ci].val, dev->command[ci].op_code));
    }
  }
  return ok;
}

bool mgos_i2c_write(struct mgos_i2c *c, uint16_t addr, const void *data,
                    size_t len, bool stop) {
  bool ok = true;
  int di = 0, ci = 0;
  i2c_dev_t *dev = c->dev;
  bool start = (addr != MGOS_I2C_ADDR_CONTINUE);
  if (c->debug) {
    LOG(LL_DEBUG,
        ("write %d to %d, start? %d stop? %d", len, addr, start, stop));
  }
  if (start) {
    esp32_i2c_write_addr(dev, addr, false /* read */, &ci);
    if (len == 0) {
      if (stop) {
        dev->command[ci++].val = I2C_COMMAND_OP_STOP;
      } else {
        dev->command[ci++].val = I2C_COMMAND_OP_END;
      }
      ok = esp32_i2c_exec(c, ci, 0, -1);
    }
  }
  bool somebody_stop_me = false;
  while (di < len && ok) {
    int tx_len = len - di;
    int tx_fifo_len = dev->status_reg.tx_fifo_cnt;
    int tx_fifo_av = (I2C_TX_FIFO_LEN - tx_fifo_len);
    if (tx_len > tx_fifo_av) tx_len = tx_fifo_av;
    bool last = ((tx_len == len - di) && stop);
    dev->command[ci++].val =
        (I2C_COMMAND_OP_WRITE | I2C_COMMAND_ACK_CHECK) | tx_len;
    if (last) {
#if 0 /* https://github.com/espressif/esp-idf/issues/606 */
      dev->command[ci++].val = I2C_COMMAND_OP_STOP;
#else
      dev->command[ci++].val = I2C_COMMAND_OP_END;
      somebody_stop_me = true;
#endif
    } else {
      dev->command[ci++].val = I2C_COMMAND_OP_END;
    }

    for (int i = 0; i < tx_len; i++, di++) {
      uint8_t byte = ((uint8_t *) data)[di];
      dev->fifo_data.val = byte;
      if (c->debug && i == 0) {
        LOG(LL_DEBUG, (" %d sent 0x%02x", di, byte));
      }
    }

    ok = esp32_i2c_exec(c, ci, 0, -1);

    ci = 0;
    dev->fifo_conf.tx_fifo_rst = true;
    dev->fifo_conf.tx_fifo_rst = false;
  }
  if (somebody_stop_me) mgos_i2c_stop(c);
  return ok;
}

bool mgos_i2c_read(struct mgos_i2c *c, uint16_t addr, void *data, size_t len,
                   bool stop) {
  bool ok = true;
  int di = 0, ci = 0;
  i2c_dev_t *dev = c->dev;
  bool start = (addr != MGOS_I2C_ADDR_CONTINUE);
  if (c->debug) {
    LOG(LL_DEBUG,
        ("read %d from %d, start? %d, stop? %d", len, addr, start, stop));
  }
  if (start) {
    dev->fifo_conf.rx_fifo_rst = true;
    dev->fifo_conf.rx_fifo_rst = false;
    esp32_i2c_write_addr(dev, addr, true /* read */, &ci);
    if (len == 0) {
      if (stop) {
        dev->command[ci++].val = I2C_COMMAND_OP_STOP;
      } else {
        dev->command[ci++].val = I2C_COMMAND_OP_END;
      }
      ok = esp32_i2c_exec(c, ci, 0, -1);
    }
  }
  bool somebody_stop_me = false;
  while (di < len && ok) {
    dev->fifo_conf.rx_fifo_rst = true;
    dev->fifo_conf.rx_fifo_rst = false;
    int rx_len = len - di;
    if (rx_len > I2C_RX_FIFO_LEN) rx_len = I2C_RX_FIFO_LEN;
    bool last = ((rx_len == len - di) && stop);
    if (last && !somebody_stop_me) {
#if 0 /* https://github.com/espressif/esp-idf/issues/606 */
      if (rx_len > 1) {
        dev->command[ci++].val = (I2C_COMMAND_OP_READ) | (rx_len - 1);
      }
      /* NAK last byte, as per spec. */
      dev->command[ci++].val =
          (I2C_COMMAND_OP_READ | I2C_COMMAND_ACK_VALUE) | 1;
#else
      dev->command[ci++].val = (I2C_COMMAND_OP_READ) | rx_len;
      dev->command[ci++].val = I2C_COMMAND_OP_END;
      somebody_stop_me = true;
#endif
    } else {
      dev->command[ci++].val = (I2C_COMMAND_OP_READ) | rx_len;
      dev->command[ci++].val = I2C_COMMAND_OP_END;
    }

    ok = esp32_i2c_exec(c, ci, -1, rx_len);

    if (ok) {
      for (int i = 0; i < rx_len; i++, di++) {
        uint8_t byte = dev->fifo_data.val;
        ((uint8_t *) data)[di] = byte;
        if (c->debug && i == 0) {
          LOG(LL_DEBUG, (" %d recd 0x%02x", di, byte));
        }
      }
    }

    ci = 0;
  }
  if (somebody_stop_me) mgos_i2c_stop(c);
  return ok;
}

void mgos_i2c_stop(struct mgos_i2c *c) {
  if (c->debug) {
    LOG(LL_DEBUG, ("stop"));
  }
  c->dev->command[0].val = I2C_COMMAND_OP_STOP;
  esp32_i2c_exec(c, 1, -1, -1);
}

int mgos_i2c_get_freq(struct mgos_i2c *c) {
  return c->freq;
}

bool mgos_i2c_set_freq(struct mgos_i2c *c, int freq) {
  i2c_dev_t *dev = c->dev;

  const uint32_t period = APB_CLK_FREQ / freq / 2;
  if (freq < 0 || period < 16) {
    /* Frequency is too high (> 2.5 MHz with APB_CLK_FREQ = 80MHz). */
    return false;
  }

  dev->scl_low_period.period = period;
  dev->scl_high_period.period = period;

  dev->scl_start_hold.time = period / 2;
  dev->scl_rstart_setup.time = period / 2;
  dev->scl_stop_hold.time = period / 2;
  dev->scl_stop_setup.time = period / 2;
  dev->sda_hold.time = period / 4;
  dev->sda_sample.time = period / 4;

  c->freq = freq;

  return true;
}

struct mgos_i2c *mgos_i2c_create(const struct sys_config_i2c *cfg) {
  struct mgos_i2c *c = NULL;
  if (cfg->sda_gpio < 0 || cfg->sda_gpio > 34 || cfg->scl_gpio < 0 ||
      cfg->scl_gpio > 34 || (cfg->unit_no != 0 && cfg->unit_no != 1) ||
      cfg->freq <= 0) {
    goto out_err;
  }

  c = calloc(1, sizeof(*c));
  if (c == NULL) return NULL;

  i2c_dev_t *dev;
  uint32_t scl_in_sig, scl_out_sig, sda_in_sig, sda_out_sig;
  if (cfg->unit_no == 0) {
    dev = (i2c_dev_t *) ESP32_I2C0_AHB_BASE;
    periph_module_enable(PERIPH_I2C0_MODULE);
    sda_in_sig = I2CEXT0_SDA_IN_IDX;
    sda_out_sig = I2CEXT0_SDA_OUT_IDX;
    scl_in_sig = I2CEXT0_SCL_IN_IDX;
    scl_out_sig = I2CEXT0_SCL_OUT_IDX;
  } else {
    dev = (i2c_dev_t *) ESP32_I2C1_AHB_BASE;
    periph_module_enable(PERIPH_I2C1_MODULE);
    sda_in_sig = I2CEXT1_SDA_IN_IDX;
    sda_out_sig = I2CEXT1_SDA_OUT_IDX;
    scl_in_sig = I2CEXT1_SCL_IN_IDX;
    scl_out_sig = I2CEXT1_SCL_OUT_IDX;
  }
  c->dev = dev;
  c->debug = cfg->debug;

  esp_err_t r = 0;
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[cfg->scl_gpio], PIN_FUNC_GPIO);
  r |= gpio_set_level(cfg->scl_gpio, 1);
  r |= gpio_set_pull_mode(cfg->scl_gpio, GPIO_PULLUP_ONLY);
  r |= gpio_set_direction(cfg->scl_gpio, GPIO_MODE_INPUT_OUTPUT_OD);
  gpio_matrix_out(cfg->scl_gpio, scl_out_sig, false /* out_inv */,
                  false /* oen_inv */);
  gpio_matrix_in(cfg->scl_gpio, scl_in_sig, false /* inv */);

  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[cfg->sda_gpio], PIN_FUNC_GPIO);
  r |= gpio_set_level(cfg->sda_gpio, 1);
  r |= gpio_set_pull_mode(cfg->sda_gpio, GPIO_PULLUP_ONLY);
  r |= gpio_set_direction(cfg->sda_gpio, GPIO_MODE_INPUT_OUTPUT_OD);
  gpio_matrix_out(cfg->sda_gpio, sda_out_sig, false /* out_inv */,
                  false /* oen_inv */);
  gpio_matrix_in(cfg->sda_gpio, sda_in_sig, false /* inv */);

  dev->ctr.val = 0;
  dev->ctr.scl_force_out = true;
  dev->ctr.sda_force_out = true;
  dev->ctr.ms_mode = true; /* Master */
  dev->ctr.clk_en = true;
  dev->fifo_conf.nonfifo_en = false; /* FIFO mode */
  dev->fifo_conf.tx_fifo_empty_thrhd = 0;
  dev->int_ena.val = 0; /* No interrupts */

  if (!mgos_i2c_set_freq(c, cfg->freq)) {
    goto out_err;
  }

  dev->timeout.tout = 20000;  // I2C_FIFO_LEN * 9 * 2 * period + 5 * period;

  LOG(LL_INFO, ("I2C%d init ok (SDA: %d, SCL: %d, freq: %d)", cfg->unit_no,
                cfg->sda_gpio, cfg->scl_gpio, c->freq));

  return c;

out_err:
  free(c);
  LOG(LL_ERROR, ("Invalid I2C settings"));
  return NULL;
}

void mgos_i2c_close(struct mgos_i2c *c) {
  mgos_i2c_stop(c);
  free(c);
}
