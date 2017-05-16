/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_features.h"

#if MGOS_ENABLE_I2C

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oslib/osi.h"

#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_i2c.h"
#include "i2c.h"
#include "pin.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"

#include "fw/src/mgos_i2c.h"
#include "config.h"

/* Documentation: TRM (swru367b), Chapter 7 */

#ifdef CC3200_I2C_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

struct mgos_i2c {
  unsigned long base;
  int freq;
};

static void reset_pin_mode_if_i2c(int pin, int i2c_mode) {
  if (MAP_PinModeGet(pin) == i2c_mode) {
    /* GPIO is always mode 0 */
    MAP_PinModeSet(pin, PIN_MODE_0);
  }
}

int mgos_i2c_get_freq(struct mgos_i2c *c) {
  return c->freq;
}

bool mgos_i2c_set_freq(struct mgos_i2c *c, int freq) {
  (void) c;
  if (freq != MGOS_I2C_FREQ_100KHZ && freq != MGOS_I2C_FREQ_400KHZ) {
    return false;
  }
  c->freq = freq;
  MAP_I2CMasterInitExpClk(c->base, SYS_CLK, (c->freq == MGOS_I2C_FREQ_400KHZ));
  return true;
}

struct mgos_i2c *mgos_i2c_create(const struct sys_config_i2c *cfg) {
  struct mgos_i2c *c = (struct mgos_i2c *) calloc(1, sizeof(*c));
  if (c == NULL) return NULL;

  c->base = I2CA0_BASE;
  int sda_pin = cfg->sda_pin - 1;
  int scl_pin = cfg->scl_pin - 1;

  int mode;
  if (scl_pin == PIN_01) {
    mode = PIN_MODE_1;
    reset_pin_mode_if_i2c(PIN_03, PIN_MODE_5);
    reset_pin_mode_if_i2c(PIN_05, PIN_MODE_5);
    reset_pin_mode_if_i2c(PIN_16, PIN_MODE_9);
  } else if (scl_pin == PIN_03 || scl_pin == PIN_05) {
    mode = PIN_MODE_5;
    reset_pin_mode_if_i2c(PIN_01, PIN_MODE_1);
    reset_pin_mode_if_i2c((scl_pin == PIN_03 ? PIN_05 : PIN_03), PIN_MODE_5);
    reset_pin_mode_if_i2c(PIN_16, PIN_MODE_9);
  } else if (scl_pin == PIN_16) {
    mode = PIN_MODE_9;
    reset_pin_mode_if_i2c(PIN_01, PIN_MODE_1);
    reset_pin_mode_if_i2c(PIN_03, PIN_MODE_5);
    reset_pin_mode_if_i2c(PIN_05, PIN_MODE_5);
  } else {
    goto out_err;
  }
  MAP_PinTypeI2C(scl_pin, mode);

  if (sda_pin == PIN_02) {
    mode = PIN_MODE_1;
    reset_pin_mode_if_i2c(PIN_04, PIN_MODE_5);
    reset_pin_mode_if_i2c(PIN_06, PIN_MODE_5);
    reset_pin_mode_if_i2c(PIN_17, PIN_MODE_9);
  } else if (sda_pin == PIN_04 || sda_pin == PIN_06) {
    mode = PIN_MODE_5;
    reset_pin_mode_if_i2c(PIN_02, PIN_MODE_1);
    reset_pin_mode_if_i2c((scl_pin == PIN_04 ? PIN_06 : PIN_04), PIN_MODE_5);
    reset_pin_mode_if_i2c(PIN_17, PIN_MODE_9);
  } else if (sda_pin == PIN_17) {
    mode = PIN_MODE_9;
    reset_pin_mode_if_i2c(PIN_02, PIN_MODE_1);
    reset_pin_mode_if_i2c(PIN_04, PIN_MODE_5);
    reset_pin_mode_if_i2c(PIN_06, PIN_MODE_5);
  } else {
    goto out_err;
  }
  MAP_PinTypeI2C(sda_pin, mode);

  MAP_PRCMPeripheralClkEnable(PRCM_I2CA0, PRCM_RUN_MODE_CLK);
  MAP_PRCMPeripheralReset(PRCM_I2CA0);

  if (!mgos_i2c_set_freq(c, cfg->freq)) {
    goto out_err;
  }

  LOG(LL_INFO, ("I2C initialized (SDA: %d, SCL: %d, freq %d)", cfg->sda_pin,
                cfg->scl_pin, cfg->freq));
  return c;

out_err:
  free(c);
  LOG(LL_ERROR, ("Invalid I2C pin settings"));
  return NULL;
}

void mgos_i2c_close(struct mgos_i2c *c) {
  MAP_PRCMPeripheralClkDisable(PRCM_I2CA0, PRCM_RUN_MODE_CLK);
  free(c);
}

/* Sends command to the I2C module and waits for it to be processed. */
static bool cc3200_i2c_command(struct mgos_i2c *c, uint32_t cmd) {
  dprintf(("-- cmd %02x\n", cmd));
  // I2CMasterTimeoutSet(c->base, 0x20); /* 5 ms @ 100 KHz */
  MAP_I2CMasterIntClearEx(I2CA0_BASE, I2C_INT_MASTER);
  I2CMasterControl(c->base, cmd);
  while (!(MAP_I2CMasterIntStatusEx(I2CA0_BASE, false) & I2C_INT_MASTER)) {
  }
  volatile uint32_t mcs = HWREG(c->base + I2C_O_MCS);
  dprintf(("mcs %02x\n", mcs));
  if ((mcs & (I2C_MCS_ARBLST | I2C_MCS_ACK | I2C_MCS_ADRACK | I2C_MCS_CLKTO)) ==
      0) {
    return true;
  } else {
    /*
     * We should release the bus is an error occurs, except if the error is lost
     * arbitration, in which case we reset the error condition in the module but
     * not touch the bus.
     * SEND and RECEIVE varieties of commands are the same, so it doesn't matter
     * (I2C_MASTER_CMD_BURST_SEND_ERROR_STOP ==
     *  I2C_MASTER_CMD_BURST_RECEIVE_ERROR_STOP == 4).
     */
    if (mcs & I2C_MCS_ARBLST) {
      MAP_I2CMasterControl(c->base, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);
    } else {
      MAP_I2CMasterControl(c->base, I2C_MASTER_CMD_BURST_SEND_FINISH);
    }
    return false;
  }
}

bool mgos_i2c_write(struct mgos_i2c *c, uint16_t addr, const void *data,
                    size_t len, bool stop) {
  dprintf(("- send 0x%02x %u %d\n", addr, len, stop));
  uint8_t *p = (uint8_t *) data;
  uint32_t cmd;
  /* Cannot transmit address only, CC3200 always transmits at least one byte. */
  if (len == 0) return false;
  if (addr == MGOS_I2C_ADDR_CONTINUE) {
    if (!MAP_I2CMasterBusBusy(c->base)) return false;
    cmd = I2C_MASTER_CMD_BURST_SEND_CONT;
  } else if (addr <= 0x7F) {
    MAP_I2CMasterSlaveAddrSet(c->base, addr, false /* read */);
    cmd = (len == 1 && stop ? I2C_MASTER_CMD_SINGLE_SEND
                            : I2C_MASTER_CMD_BURST_SEND_START);
  } else {
    /* CC3200 does not support 10 bit addresses. */
    return false;
  }
  while (len-- > 0) {
    MAP_I2CMasterDataPut(c->base, *p++);
    if (!cc3200_i2c_command(c, cmd)) return false;
    cmd = (len == 1 && stop ? I2C_MASTER_CMD_BURST_SEND_FINISH
                            : I2C_MASTER_CMD_BURST_SEND_CONT);
  }
  return true;
}

bool mgos_i2c_read(struct mgos_i2c *c, uint16_t addr, void *data, size_t len,
                   bool stop) {
  uint8_t *p = (uint8_t *) data;
  uint32_t cmd;
  dprintf(("- recv 0x%02x %u %d\n", addr, len, stop));
  if (len == 0) return false;
  if (addr == MGOS_I2C_ADDR_CONTINUE) {
    if (!MAP_I2CMasterBusBusy(c->base)) return false;
    cmd = I2C_MASTER_CMD_BURST_RECEIVE_CONT;
  } else if (addr <= 0x7F) {
    MAP_I2CMasterSlaveAddrSet(c->base, addr, true /* read */);
    cmd = (len == 1 && stop ? I2C_MASTER_CMD_SINGLE_RECEIVE
                            : I2C_MASTER_CMD_BURST_RECEIVE_START);
  } else {
    /* CC3200 does not support 10 bit addresses. */
    return false;
  }
  while (len-- > 0) {
    if (!cc3200_i2c_command(c, cmd)) return false;
    *p++ = MAP_I2CMasterDataGet(c->base);
    cmd = (len == 1 && stop ? I2C_MASTER_CMD_BURST_RECEIVE_FINISH
                            : I2C_MASTER_CMD_BURST_RECEIVE_CONT);
  }
  return true;
}

void mgos_i2c_stop(struct mgos_i2c *c) {
  dprintf(("stop mcs %lx\n", HWREG(c->base + I2C_O_MCS)));
  if (MAP_I2CMasterBusBusy(c->base)) {
    cc3200_i2c_command(c, I2C_MASTER_CMD_BURST_SEND_FINISH);
  }
}

#endif /* MGOS_ENABLE_I2C */
