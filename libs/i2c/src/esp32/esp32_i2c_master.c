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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "mgos_i2c.h"

#include "esp_attr.h"
#include "driver/gpio.h"
#include "driver/periph_ctrl.h"
#include "soc/gpio_sig_map.h"
#include "soc/i2c_reg.h"
#include "soc/i2c_struct.h"

#include "mgos_sys_config.h"
#include "mgos_system.h"

#include "common/cs_dbg.h"

#define I2C_COMMAND_OP_RSTART (0 << 11)
#define I2C_COMMAND_OP_WRITE (1 << 11)
#define I2C_COMMAND_OP_READ (2 << 11)
#define I2C_COMMAND_OP_STOP (3 << 11)
#define I2C_COMMAND_OP_END (4 << 11)
#define I2C_COMMAND_ACK_VALUE (1 << 10)
#define I2C_COMMAND_ACK_EXP (1 << 9)
#define I2C_COMMAND_ACK_CHECK (1 << 8)

#define I2C_FIFO_LEN 31

/* We use AHB addr to avoid https://github.com/espressif/esp-idf/issues/595 */
#define ESP32_I2C0_AHB_BASE 0x60013000
#define ESP32_I2C1_AHB_BASE 0x60027000

/*
 * Using the END command is very tricky. Here's what we know:
 *  - Other command cannot be used in the same slot where END once was:
 *    S, W, W, E,
 *    W, E,
 *    W, W!, E
 *    This is not ok because W! is in the same slot as E was previosuly.
 * - Zero-length reads and writes can be used to pad the pipeline, but..
 * - Transitions from read to zero-length write and vice-versa are not allowed:
 *   S, W0, W, R0, R, E - not ok, transitions from non-zero write to zero read.
 *   S, W0, W, R, R0, E - ok
 *   S, W, W0, R, R0, E - not ok, transitions from W0 to R.
 * We use read and write NOPs to make sure END is always at a fixed position in
 * in the command pipeline.
 */
#define I2C_COMMAND_NOP_R (I2C_COMMAND_OP_READ)
#define I2C_COMMAND_NOP_W (I2C_COMMAND_OP_WRITE)
#define END_POS 5

#define I2C_DONE_INTS (I2C_TRANS_COMPLETE_INT_RAW | I2C_END_DETECT_INT_RAW)
#define I2C_ERROR_INTS                                                         \
  (I2C_ACK_ERR_INT_RAW | I2C_TIME_OUT_INT_RAW | I2C_ARBITRATION_LOST_INT_RAW | \
   I2C_RXFIFO_OVF_INT_RAW)

struct mgos_i2c {
  struct mgos_config_i2c cfg;
  uint32_t scl_in_sig, scl_out_sig;
  uint32_t sda_in_sig, sda_out_sig;
  periph_module_t pm;
  i2c_dev_t *dev;
  int freq;
  bool debug;
  bool stopped;
};

bool mgos_i2c_set_freq(struct mgos_i2c *c, int freq) {
  i2c_dev_t *dev = c->dev;

  const uint32_t period = APB_CLK_FREQ / freq;
  const uint32_t half_period = period / 2;

  if (c->debug) {
    LOG(LL_DEBUG, ("freq %d period %d", freq, (int) period));
  }

  if (freq < 0 || half_period < 40) {
    /* Frequency is too high (> 2 MHz with APB_CLK_FREQ = 80MHz). */
    return false;
  }

  dev->scl_low_period.period = half_period;
  /*
   * 35 APB cycles is the 440uS correction to observed constant difference
   * in SCL high time.
   */
  dev->scl_high_period.period = half_period - 35;

  dev->scl_start_hold.time = half_period;
  dev->scl_rstart_setup.time = half_period;
  dev->scl_stop_hold.time = half_period;
  dev->scl_stop_setup.time = half_period;
  dev->sda_hold.time = half_period / 2;
  dev->sda_sample.time = half_period / 2;

  dev->timeout.tout = 32000;  // I2C_FIFO_LEN * 9 * 2 * period + 5 * period;

  c->freq = freq;

  return true;
}

static bool esp32_i2c_reset(struct mgos_i2c *c, int new_freq) {
  i2c_dev_t *dev = c->dev;

  /* Disconnect from output while resetting to avoid glitch on the bus. */
  gpio_matrix_out(c->cfg.scl_gpio, SIG_GPIO_OUT_IDX, false /* out_inv */,
                  false /* oen_inv */);
  gpio_matrix_out(c->cfg.sda_gpio, SIG_GPIO_OUT_IDX, false /* out_inv */,
                  false /* oen_inv */);

  periph_module_disable(c->pm);
  mgos_msleep(1);
  periph_module_enable(c->pm);
  mgos_msleep(1);

  dev->ctr.val = 0;
  /* MSB first for RX and TX */
  dev->ctr.rx_lsb_first = false;
  dev->ctr.tx_lsb_first = false;
  /* OD for SCL and SDA */
  dev->ctr.scl_force_out = true;
  dev->ctr.sda_force_out = true;
  dev->ctr.ms_mode = true;           /* Master */
  dev->fifo_conf.nonfifo_en = false; /* FIFO mode */
  dev->fifo_conf.tx_fifo_empty_thrhd = 0;
  dev->int_ena.val = 0; /* No interrupts */

  gpio_matrix_out(c->cfg.scl_gpio, c->scl_out_sig, false /* out_inv */,
                  false /* oen_inv */);
  gpio_matrix_out(c->cfg.sda_gpio, c->sda_out_sig, false /* out_inv */,
                  false /* oen_inv */);

  c->stopped = true;

  return mgos_i2c_set_freq(c, new_freq);
}

static void esp32_i2c_write_addr(struct mgos_i2c *c, uint16_t addr, bool read,
                                 int pad, int *ci) {
  if (addr == MGOS_I2C_ADDR_CONTINUE) return;
  i2c_dev_t *dev = c->dev;
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
  while (pad-- > 0) dev->command[(*ci)++].val = I2C_COMMAND_NOP_W;
  dev->command[(*ci)++].val =
      (I2C_COMMAND_OP_WRITE | I2C_COMMAND_ACK_CHECK) | addr_len;
}

static void esp32_i2c_finish_cmd(struct mgos_i2c *c, int *ci, bool stop) {
  if (stop) {
    c->dev->command[(*ci)++].val = I2C_COMMAND_OP_STOP;
    c->stopped = true;
  } else {
    assert(*ci == END_POS);
    c->dev->command[(*ci)++].val = I2C_COMMAND_OP_END;
    c->stopped = false;
  }
}

static bool esp32_i2c_exec(struct mgos_i2c *c, int num_cmds, int exp_tx,
                           int exp_rx) {
  i2c_dev_t *dev = c->dev;
  int loops = 0;
  bool ok = false;
  uint32_t ints = dev->int_raw.val;
  dev->int_clr.val = 0x1fff;
  uint32_t ints_before = dev->int_raw.val;
  dev->ctr.trans_start = true;
  uint32_t old_fifo_cnt =
      dev->status_reg.tx_fifo_cnt + dev->status_reg.rx_fifo_cnt;
  uint32_t max_loops = 20000;
  do {
    loops++;
    ints = dev->int_raw.val;
    uint32_t fifo_cnt =
        dev->status_reg.tx_fifo_cnt + dev->status_reg.rx_fifo_cnt;
    if (fifo_cnt != old_fifo_cnt) {
      max_loops += 20000;
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
    for (int ci = 0; ci < num_cmds; ci++) {
      LOG(LL_DEBUG,
          ("    %08x %d", dev->command[ci].val, dev->command[ci].op_code));
    }
  }
  if (loops >= max_loops || (ints & I2C_ERROR_INTS)) {
    /*
     * Something went wrong, reset the unit.
     * In particular, recovering from timeout and lost arbitration errors
     * often requires a reset.
     */
    esp32_i2c_reset(c, c->freq);
  }
  return ok;
}

bool mgos_i2c_write(struct mgos_i2c *c, uint16_t addr, const void *data,
                    size_t len, bool stop) {
  bool ok = true;
  int di = 0, ci = 0;
  i2c_dev_t *dev = c->dev;
  if (c->debug) {
    LOG(LL_DEBUG, ("write %d to %d, stop? %d", len, addr, stop));
  }
  esp32_i2c_write_addr(c, addr, false /* read */, (len == 0 ? 3 : 0), &ci);
  if (len == 0 && (ci > 0 || (stop && !c->stopped))) {
    esp32_i2c_finish_cmd(c, &ci, stop);
    ok = esp32_i2c_exec(c, ci, 0, -1);
  } else {
    while (di < len && ok) {
      int tx_len = len - di;
      int tx_fifo_len = dev->status_reg.tx_fifo_cnt;
      int tx_fifo_av = (I2C_FIFO_LEN - tx_fifo_len);
      if (tx_len > tx_fifo_av) tx_len = tx_fifo_av;
      bool last = ((tx_len == len - di) && stop);
      if (!last) {
        while (ci < END_POS - 1) dev->command[ci++].val = I2C_COMMAND_NOP_W;
      }
      dev->command[ci++].val =
          (I2C_COMMAND_OP_WRITE | I2C_COMMAND_ACK_CHECK) | tx_len;
      esp32_i2c_finish_cmd(c, &ci, last);

      for (int i = 0; i < tx_len; i++, di++) {
        uint8_t byte = ((uint8_t *) data)[di];
        dev->fifo_data.val = byte;
      }
      ok = esp32_i2c_exec(c, ci, 0, -1);

      ci = 0;
      dev->fifo_conf.tx_fifo_rst = true;
      dev->fifo_conf.tx_fifo_rst = false;
    }
  }
  return ok;
}

bool mgos_i2c_read(struct mgos_i2c *c, uint16_t addr, void *data, size_t len,
                   bool stop) {
  bool ok = true;
  int di = 0, ci = 0;
  i2c_dev_t *dev = c->dev;
  if (c->debug) {
    LOG(LL_DEBUG, ("read %d from %d, stop? %d", len, addr, stop));
  }
  esp32_i2c_write_addr(c, addr, true /* read */, (len == 0 ? 3 : 0), &ci);
  if (len == 0 && (ci > 0 || (stop && !c->stopped))) {
    esp32_i2c_finish_cmd(c, &ci, stop);
    ok = esp32_i2c_exec(c, ci, 0, -1);
  } else {
    while (di < len && ok) {
      dev->fifo_conf.rx_fifo_rst = true;
      dev->fifo_conf.rx_fifo_rst = false;
      int rx_len = len - di;
      if (rx_len > I2C_FIFO_LEN) rx_len = I2C_FIFO_LEN;
      bool last = ((rx_len == len - di) && stop);
      if (!last) {
        dev->command[ci++].val = I2C_COMMAND_OP_READ | rx_len;
        while (ci < END_POS) dev->command[ci++].val = I2C_COMMAND_NOP_R;
      } else {
        if (rx_len > 1) {
          dev->command[ci++].val = I2C_COMMAND_OP_READ | (rx_len - 1);
        }
        /* NAK last byte, as per spec. */
        dev->command[ci++].val =
            I2C_COMMAND_OP_READ | I2C_COMMAND_ACK_VALUE | 1;
      }
      esp32_i2c_finish_cmd(c, &ci, last);

      ok = esp32_i2c_exec(c, ci, -1, rx_len);

      if (ok) {
        for (int i = 0; i < rx_len; i++, di++) {
          uint8_t byte = dev->fifo_data.val;
          ((uint8_t *) data)[di] = byte;
        }
      }

      ci = 0;
    }
  }
  return ok;
}

void mgos_i2c_stop(struct mgos_i2c *c) {
  if (c->stopped) return;
  if (c->debug) {
    LOG(LL_DEBUG, ("stop"));
  }
  c->dev->command[0].val = I2C_COMMAND_OP_STOP;
  esp32_i2c_exec(c, 1, -1, -1);
}

int mgos_i2c_get_freq(struct mgos_i2c *c) {
  return c->freq;
}

struct mgos_i2c *mgos_i2c_create(const struct mgos_config_i2c *cfg) {
  struct mgos_i2c *c = NULL;
  if (cfg->sda_gpio < 0 || cfg->sda_gpio > 33 || cfg->scl_gpio < 0 ||
      cfg->scl_gpio > 33 || (cfg->unit_no != 0 && cfg->unit_no != 1) ||
      cfg->freq <= 0) {
    goto out_err;
  }

  c = calloc(1, sizeof(*c));
  if (c == NULL) return NULL;

  memcpy(&c->cfg, cfg, sizeof(c->cfg));

  if (!mgos_i2c_reset_bus(c->cfg.sda_gpio, c->cfg.scl_gpio)) {
    goto out_err;
  }

  if (cfg->unit_no == 0) {
    c->pm = PERIPH_I2C0_MODULE;
    c->dev = (i2c_dev_t *) ESP32_I2C0_AHB_BASE;
    c->sda_in_sig = I2CEXT0_SDA_IN_IDX;
    c->sda_out_sig = I2CEXT0_SDA_OUT_IDX;
    c->scl_in_sig = I2CEXT0_SCL_IN_IDX;
    c->scl_out_sig = I2CEXT0_SCL_OUT_IDX;
  } else {
    c->pm = PERIPH_I2C1_MODULE;
    c->dev = (i2c_dev_t *) ESP32_I2C1_AHB_BASE;
    c->sda_in_sig = I2CEXT1_SDA_IN_IDX;
    c->sda_out_sig = I2CEXT1_SDA_OUT_IDX;
    c->scl_in_sig = I2CEXT1_SCL_IN_IDX;
    c->scl_out_sig = I2CEXT1_SCL_OUT_IDX;
  }
  c->debug = cfg->debug;

  if (!esp32_i2c_reset(c, cfg->freq)) goto out_err;

  esp_err_t r = 0;
  r |= gpio_set_level(c->cfg.scl_gpio, 1);
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[c->cfg.scl_gpio], PIN_FUNC_GPIO);
  r |= gpio_set_direction(c->cfg.scl_gpio, GPIO_MODE_INPUT_OUTPUT_OD);
  gpio_matrix_out(c->cfg.scl_gpio, c->scl_out_sig, false /* out_inv */,
                  false /* oen_inv */);
  r |= gpio_set_pull_mode(c->cfg.scl_gpio, GPIO_PULLUP_ONLY);
  gpio_matrix_in(c->cfg.scl_gpio, c->scl_in_sig, false /* inv */);

  r |= gpio_set_level(c->cfg.sda_gpio, 1);
  PIN_FUNC_SELECT(GPIO_PIN_MUX_REG[c->cfg.sda_gpio], PIN_FUNC_GPIO);
  r |= gpio_set_pull_mode(c->cfg.sda_gpio, GPIO_PULLUP_ONLY);
  r |= gpio_set_direction(c->cfg.sda_gpio, GPIO_MODE_INPUT_OUTPUT_OD);
  gpio_matrix_out(c->cfg.sda_gpio, c->sda_out_sig, false /* out_inv */,
                  false /* oen_inv */);
  gpio_matrix_in(c->cfg.sda_gpio, c->sda_in_sig, false /* inv */);

  if (r != ESP_OK) {
    LOG(LL_ERROR, ("Failed to configure pins"));
    goto out_err;
  }

  LOG(LL_INFO, ("I2C%d init ok (SDA: %d, SCL: %d, freq: %d)", c->cfg.unit_no,
                c->cfg.sda_gpio, c->cfg.scl_gpio, c->freq));

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
