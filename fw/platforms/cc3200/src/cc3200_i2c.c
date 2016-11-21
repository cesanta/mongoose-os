/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/platforms/esp8266/user/esp_features.h"

#if MIOT_ENABLE_I2C

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

#include "fw/src/miot_i2c.h"
#include "config.h"

#if MIOT_ENABLE_JS
#include "v7/v7.h"
#endif

/* Documentation: TRM (swru367b), Chapter 7 */

#ifdef CC3200_I2C_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

struct miot_i2c {
  unsigned long base;
  uint8_t first : 1;
};

struct miot_i2c *miot_i2c_create(const struct sys_config_i2c *cfg) {
  struct miot_i2c *c = (struct miot_i2c *) calloc(1, sizeof(*c));
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

void miot_i2c_close(struct miot_i2c *c) {
  MAP_PRCMPeripheralClkDisable(PRCM_I2CA0, PRCM_RUN_MODE_CLK);
  free(c);
}

/* Sends command to the I2C module and waits for it to be processed. */
static enum i2c_ack_type cc3200_i2c_command(struct miot_i2c *c, uint32_t cmd) {
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

enum i2c_ack_type miot_i2c_start(struct miot_i2c *c, uint16_t addr,
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

enum i2c_ack_type miot_i2c_send_byte(struct miot_i2c *c, uint8_t data) {
  I2CMasterDataPut(c->base, data);
  if (c->first) {
    c->first = 0;
    return cc3200_i2c_command(c, I2C_MASTER_CMD_BURST_SEND_START);
  } else {
    return cc3200_i2c_command(c, I2C_MASTER_CMD_BURST_SEND_CONT);
  }
}

uint8_t miot_i2c_read_byte(struct miot_i2c *c, enum i2c_ack_type ack_type) {
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

void miot_i2c_stop(struct miot_i2c *c) {
  dprintf(("stop mcs %lx\n", HWREG(c->base + I2C_O_MCS)));
  if (I2CMasterBusBusy(c->base)) {
    cc3200_i2c_command(c, I2C_MASTER_CMD_BURST_SEND_FINISH);
  }
}

void miot_i2c_send_ack(struct miot_i2c *c, enum i2c_ack_type ack_type) {
}

#if MIOT_ENABLE_JS && MIOT_ENABLE_I2C_API
enum v7_err miot_i2c_create_js(struct v7 *v7, struct miot_i2c **res) {
  enum v7_err rcode = V7_OK;
  struct sys_config_i2c cfg;
  cfg.sda_pin = v7_get_double(v7, v7_arg(v7, 0)) - 1;
  cfg.scl_pin = v7_get_double(v7, v7_arg(v7, 1)) - 1;
  struct miot_i2c *conn = miot_i2c_create(&cfg);

  if (conn != NULL) {
    *res = conn;
  } else {
    rcode = v7_throwf(v7, "Error", "Failed to creat I2C connection");
  }

  return rcode;
}
#endif /* MIOT_ENABLE_JS && MIOT_ENABLE_I2C_API */

#endif /* MIOT_ENABLE_I2C */
