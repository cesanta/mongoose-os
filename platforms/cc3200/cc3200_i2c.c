#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include "oslib/osi.h"

#include "hw_types.h"
#include "hw_memmap.h"
#include "hw_i2c.h"
typedef char bool;
#include "i2c.h"
#include "pin.h"
#include "prcm.h"
#include "rom.h"
#include "rom_map.h"

#include "sj_i2c.h"
#include "v7.h"
#include "config.h"

/* Documentation: TRM (swru367b), Chapter 7 */

#ifdef CC3200_I2C_DEBUG
#define dprintf(x) printf x
#else
#define dprintf(x)
#endif

struct i2c_state {
  unsigned long base;
  unsigned long sda_pin, sda_pin_mode, sda_pin_strength, sda_pin_type;
  unsigned long scl_pin, scl_pin_mode, scl_pin_strength, scl_pin_type;
  uint8_t first : 1;
};

i2c_connection sj_i2c_create(struct v7 *v7) {
  struct i2c_state *c = calloc(1, sizeof(struct i2c_state));
  c->sda_pin = I2C_SDA_PIN;
  c->scl_pin = I2C_SCL_PIN;
  return c;
}

int i2c_init(i2c_connection conn) {
  struct i2c_state *c = (struct i2c_state *) conn;
  c->base = I2CA0_BASE;
  c->sda_pin_mode = MAP_PinModeGet(c->scl_pin);
  MAP_PinConfigGet(c->sda_pin, &c->sda_pin_strength, &c->sda_pin_type);
  MAP_PinTypeI2C(c->sda_pin, PIN_MODE_1); /* SDA */
  c->scl_pin_mode = MAP_PinModeGet(c->scl_pin);
  MAP_PinConfigGet(c->scl_pin, &c->scl_pin_strength, &c->scl_pin_type);
  MAP_PinTypeI2C(c->scl_pin, PIN_MODE_1); /* SCL */
  MAP_PRCMPeripheralClkEnable(PRCM_I2CA0, PRCM_RUN_MODE_CLK);
  MAP_PRCMPeripheralReset(PRCM_I2CA0);
  MAP_I2CMasterInitExpClk(c->base, SYS_CLK, 0 /* 100 KHz */);
  return 0;
}

void sj_i2c_close(i2c_connection conn) {
  struct i2c_state *c = (struct i2c_state *) conn;
  MAP_PRCMPeripheralClkDisable(PRCM_I2CA0, PRCM_RUN_MODE_CLK);
  MAP_PinModeSet(c->sda_pin, c->sda_pin_mode);
  MAP_PinConfigSet(c->sda_pin, c->sda_pin_strength, c->sda_pin_type);
  MAP_PinModeSet(c->scl_pin, c->scl_pin_mode);
  MAP_PinConfigSet(c->scl_pin, c->scl_pin_strength, c->scl_pin_type);
  free(c);
}

/* Sends command to the I2C module and waits for it to be processed. */
static int i2c_command(struct i2c_state *c, uint32_t cmd) {
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
