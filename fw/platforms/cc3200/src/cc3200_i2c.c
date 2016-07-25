/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

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

#include "fw/src/sj_i2c.h"
#include "config.h"

#ifdef SJ_ENABLE_JS
#include "v7/v7.h"
#endif

/* Documentation: TRM (swru367b), Chapter 7 */

#ifdef CC3200_I2C_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

struct i2c_state {
  unsigned long base;
  unsigned long sda_pin;
  unsigned long scl_pin;
  uint8_t first : 1;
};

#ifdef SJ_ENABLE_JS
enum v7_err sj_i2c_create(struct v7 *v7, i2c_connection *res) {
  struct i2c_state *c = calloc(1, sizeof(struct i2c_state));
  c->sda_pin = v7_get_double(v7, v7_arg(v7, 0)) - 1;
  if (c->sda_pin <= 0) c->sda_pin = PIN_02;
  c->scl_pin = v7_get_double(v7, v7_arg(v7, 1)) - 1;
  if (c->scl_pin <= 0) c->scl_pin = PIN_01;
  *res = c;
  return V7_OK;
}
#endif

int i2c_init(i2c_connection conn) {
  struct i2c_state *c = (struct i2c_state *) conn;
  c->base = I2CA0_BASE;

  int mode;
  if (c->scl_pin == PIN_01) {
    mode = PIN_MODE_1;
  } else if (c->scl_pin == PIN_03 || c->sda_pin == PIN_05) {
    mode = PIN_MODE_5;
  } else if (c->sda_pin == PIN_16) {
    mode = PIN_MODE_9;
  } else {
    return -1;
  }
  MAP_PinTypeI2C(c->scl_pin, mode);

  if (c->sda_pin == PIN_02) {
    mode = PIN_MODE_1;
  } else if (c->sda_pin == PIN_04 || c->sda_pin == PIN_06) {
    mode = PIN_MODE_5;
  } else if (c->sda_pin == PIN_17) {
    mode = PIN_MODE_9;
  } else {
    return -1;
  }
  MAP_PinTypeI2C(c->sda_pin, mode);

  MAP_PRCMPeripheralClkEnable(PRCM_I2CA0, PRCM_RUN_MODE_CLK);
  MAP_PRCMPeripheralReset(PRCM_I2CA0);
  MAP_I2CMasterInitExpClk(c->base, SYS_CLK, 0 /* 100 KHz */);
  return 0;
}

void sj_i2c_close(i2c_connection conn) {
  struct i2c_state *c = (struct i2c_state *) conn;
  MAP_PRCMPeripheralClkDisable(PRCM_I2CA0, PRCM_RUN_MODE_CLK);
  free(c);
}

/* Sends command to the I2C module and waits for it to be processed. */
static enum i2c_ack_type i2c_command(struct i2c_state *c, uint32_t cmd) {
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

enum i2c_ack_type i2c_start(i2c_connection conn, uint16_t addr,
                            enum i2c_rw mode) {
  struct i2c_state *c = (struct i2c_state *) conn;
  /* CC3200 does not support 10 bit addresses. */
  if (addr > 0x7F) return I2C_ERR;
  MAP_I2CMasterSlaveAddrSet(c->base, addr, (mode == I2C_READ));
  c->first = 1;
  if (mode == I2C_WRITE) {
    /* Start condition is set only with data to send. */
    return I2C_ACK;
  } else {
    c->first = 1;
    return i2c_command(c, I2C_MASTER_CMD_BURST_RECEIVE_START);
  }
}

enum i2c_ack_type i2c_send_byte(i2c_connection conn, uint8_t data) {
  struct i2c_state *c = (struct i2c_state *) conn;
  I2CMasterDataPut(c->base, data);
  if (c->first) {
    c->first = 0;
    return i2c_command(c, I2C_MASTER_CMD_BURST_SEND_START);
  } else {
    return i2c_command(c, I2C_MASTER_CMD_BURST_SEND_CONT);
  }
}

enum i2c_ack_type i2c_send_bytes(i2c_connection conn, uint8_t *buf,
                                 size_t buf_size) {
  for (size_t i = 0; i < buf_size; i++) {
    if (i2c_send_byte(conn, buf[i]) == I2C_NAK) return I2C_NAK;
  }
  return I2C_ACK;
}

uint8_t i2c_read_byte(i2c_connection conn, enum i2c_ack_type ack_type) {
  struct i2c_state *c = (struct i2c_state *) conn;
  if (c->first) {
    /* First byte is buffered since the time of start. */
    c->first = 0;
  } else {
    i2c_command(c, ack_type == I2C_ACK ? I2C_MASTER_CMD_BURST_RECEIVE_CONT
                                       : I2C_MASTER_CMD_BURST_RECEIVE_FINISH);
  }
  return (uint8_t) I2CMasterDataGet(c->base);
}

void i2c_read_bytes(i2c_connection conn, size_t n, uint8_t *buf,
                    enum i2c_ack_type last_ack_type) {
  uint8_t *p = buf;
  while (n > 1) {
    *p++ = i2c_read_byte(conn, I2C_ACK);
    n--;
  }
  if (n == 1) *p = i2c_read_byte(conn, last_ack_type);
}

void i2c_stop(i2c_connection conn) {
  struct i2c_state *c = (struct i2c_state *) conn;
  dprintf(("stop mcs %lx\n", HWREG(c->base + I2C_O_MCS)));
  if (I2CMasterBusBusy(c->base)) {
    i2c_command(c, I2C_MASTER_CMD_BURST_SEND_FINISH);
  }
}

void i2c_send_ack(i2c_connection conn, enum i2c_ack_type ack_type) {
}
