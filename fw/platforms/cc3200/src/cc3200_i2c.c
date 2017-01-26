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
  uint8_t first : 1;
};

struct mgos_i2c *mgos_i2c_create(const struct sys_config_i2c *cfg) {
  struct mgos_i2c *c = (struct mgos_i2c *) calloc(1, sizeof(*c));
  if (c == NULL) return NULL;

  c->base = I2CA0_BASE;
  int sda_pin = cfg->sda_pin - 1;
  int scl_pin = cfg->scl_pin - 1;

  int mode;
  if (scl_pin == PIN_01) {
    mode = PIN_MODE_1;
  } else if (scl_pin == PIN_03 || sda_pin == PIN_05) {
    mode = PIN_MODE_5;
  } else if (sda_pin == PIN_16) {
    mode = PIN_MODE_9;
  } else {
    goto out_err;
  }
  MAP_PinTypeI2C(scl_pin, mode);

  if (sda_pin == PIN_02) {
    mode = PIN_MODE_1;
  } else if (sda_pin == PIN_04 || sda_pin == PIN_06) {
    mode = PIN_MODE_5;
  } else if (sda_pin == PIN_17) {
    mode = PIN_MODE_9;
  } else {
    goto out_err;
  }
  MAP_PinTypeI2C(sda_pin, mode);

  MAP_PRCMPeripheralClkEnable(PRCM_I2CA0, PRCM_RUN_MODE_CLK);
  MAP_PRCMPeripheralReset(PRCM_I2CA0);
  MAP_I2CMasterInitExpClk(c->base, SYS_CLK, cfg->fast);

  LOG(LL_INFO, ("I2C initialized (SDA: %d, SCL: %d, fast? %d)", cfg->sda_pin,
                cfg->scl_pin, cfg->fast));
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
static enum i2c_ack_type cc3200_i2c_command(struct mgos_i2c *c, uint32_t cmd) {
  I2CMasterIntClear(c->base);
  I2CMasterTimeoutSet(c->base, 0x20); /* 5 ms @ 100 KHz */
  I2CMasterControl(c->base, cmd);
  unsigned long mcs;
  while ((MAP_I2CMasterIntStatusEx(c->base, 0) &
          (I2C_MRIS_RIS | I2C_MRIS_CLKRIS)) == 0) {
    mcs = HWREG(c->base + I2C_O_MCS);
    dprintf(("busy mcs %lx cnt %lx\n", mcs, HWREG(c->base + I2C_O_MCLKOCNT)));
  }
  I2CMasterIntClearEx(c->base, I2C_MRIS_RIS | I2C_MRIS_CLKRIS);
  mcs = HWREG(c->base + I2C_O_MCS);
  dprintf(("mcs %lx\n", mcs));
  if ((mcs & (I2C_MCS_ARBLST | I2C_MCS_ACK | I2C_MCS_ADRACK | I2C_MCS_CLKTO)) ==
      0) {
    return I2C_ACK;
  } else {
    /* This does not actually put STOP condition on the bus (bit 0 = 0),
     * but resets the error condition in the module. */
    I2CMasterControl(c->base, I2C_MASTER_CMD_BURST_SEND_ERROR_STOP);
    return I2C_NAK;
  }
}

enum i2c_ack_type mgos_i2c_start(struct mgos_i2c *c, uint16_t addr,
                                 enum i2c_rw mode) {
  /* CC3200 does not support 10 bit addresses. */
  if (addr > 0x7F) return I2C_ERR;
  MAP_I2CMasterSlaveAddrSet(c->base, addr, (mode == I2C_READ));
  c->first = 1;
  if (mode == I2C_WRITE) {
    /* Start condition is set only with data to send. */
    return I2C_ACK;
  } else {
    c->first = 1;
    return cc3200_i2c_command(c, I2C_MASTER_CMD_BURST_RECEIVE_START);
  }
}

enum i2c_ack_type mgos_i2c_send_byte(struct mgos_i2c *c, uint8_t data) {
  I2CMasterDataPut(c->base, data);
  if (c->first) {
    c->first = 0;
    return cc3200_i2c_command(c, I2C_MASTER_CMD_BURST_SEND_START);
  } else {
    return cc3200_i2c_command(c, I2C_MASTER_CMD_BURST_SEND_CONT);
  }
}

uint8_t mgos_i2c_read_byte(struct mgos_i2c *c, enum i2c_ack_type ack_type) {
  if (c->first) {
    /* First byte is buffered since the time of start. */
    c->first = 0;
  } else {
    cc3200_i2c_command(c, ack_type == I2C_ACK
                              ? I2C_MASTER_CMD_BURST_RECEIVE_CONT
                              : I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
  }
  return (uint8_t) I2CMasterDataGet(c->base);
}

void mgos_i2c_stop(struct mgos_i2c *c) {
  dprintf(("stop mcs %lx\n", HWREG(c->base + I2C_O_MCS)));
  if (I2CMasterBusBusy(c->base)) {
    cc3200_i2c_command(c, I2C_MASTER_CMD_BURST_SEND_FINISH);
  }
}

void mgos_i2c_send_ack(struct mgos_i2c *c, enum i2c_ack_type ack_type) {
}

#endif /* MGOS_ENABLE_I2C */
