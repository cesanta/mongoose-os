/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_i2c.h"

#if MIOT_ENABLE_I2C

static struct miot_i2c *s_global_i2c;

void miot_i2c_read_bytes(struct miot_i2c *c, size_t n, uint8_t *buf,
                         enum i2c_ack_type last_ack_type) {
  size_t i;

  for (i = 0; i < n; i++) {
    enum i2c_ack_type ack_type = (i != n - 1 ? I2C_ACK : last_ack_type);
    *buf++ = miot_i2c_read_byte(c, ack_type);
  }
}

enum i2c_ack_type miot_i2c_send_bytes(struct miot_i2c *c, const uint8_t *buf,
                                      size_t buf_size) {
  enum i2c_ack_type ack_type = I2C_NAK;

  while (buf_size-- > 0) {
    ack_type = miot_i2c_send_byte(c, *buf++);
    if (ack_type != I2C_ACK) {
      if (buf_size != 0) {
        return I2C_ERR;
      } else {
        break;
      }
    }
  }

  return ack_type;
}

static bool miot_i2c_write_reg_addr(struct miot_i2c *conn, uint16_t addr,
                                    uint8_t reg) {
  bool res = false;
  bool started = false;
  if (miot_i2c_start(conn, addr, I2C_WRITE) != I2C_ACK) goto out;
  started = true;
  res = (miot_i2c_send_byte(conn, reg) == I2C_ACK);
out:
  if (started && !res) miot_i2c_stop(conn);
  return res;
}

static bool miot_i2c_read_reg_n(struct miot_i2c *conn, uint16_t addr,
                                uint8_t reg, size_t len, uint8_t *buf) {
  if (!miot_i2c_write_reg_addr(conn, addr, reg) ||
      (miot_i2c_start(conn, addr, I2C_READ) != I2C_ACK)) {
    return false;
  }
  miot_i2c_read_bytes(conn, len, buf, I2C_ACK);
  miot_i2c_stop(conn);
  return true;
}

bool miot_i2c_read_reg_b(struct miot_i2c *conn, uint16_t addr, uint8_t reg,
                         uint8_t *value) {
  return miot_i2c_read_reg_n(conn, addr, reg, 1, value);
}

bool miot_i2c_read_reg_w(struct miot_i2c *conn, uint16_t addr, uint8_t reg,
                         uint16_t *value) {
  uint8_t tmp[2];
  bool res = miot_i2c_read_reg_n(conn, addr, reg, 2, tmp);
  if (res) {
    *value = (((uint16_t) tmp[0]) << 8) | tmp[1];
  } else {
    *value = 0;
  }
  return res;
}

static bool miot_i2c_write_reg_n(struct miot_i2c *conn, uint16_t addr,
                                 uint8_t reg, size_t len, const uint8_t *buf) {
  bool res = false;
  if (!miot_i2c_write_reg_addr(conn, addr, reg)) return false;
  res = (miot_i2c_send_bytes(conn, buf, len) == I2C_ACK);
  miot_i2c_stop(conn);
  return res;
}

bool miot_i2c_write_reg_b(struct miot_i2c *conn, uint16_t addr, uint8_t reg,
                          uint8_t value) {
  return miot_i2c_write_reg_n(conn, addr, reg, 1, &value);
}

bool miot_i2c_write_reg_w(struct miot_i2c *conn, uint16_t addr, uint8_t reg,
                          uint16_t value) {
  uint8_t tmp[2] = {(uint8_t)(value >> 8), (uint8_t) value};
  return miot_i2c_write_reg_n(conn, addr, reg, 2, tmp);
}

enum miot_init_result miot_i2c_init(void) {
  const struct sys_config_i2c *cfg = &get_cfg()->i2c;
  if (!cfg->enable) return MIOT_INIT_OK;
  s_global_i2c = miot_i2c_create(cfg);
  return (s_global_i2c != NULL ? MIOT_INIT_OK : MIOT_INIT_I2C_FAILED);
}

struct miot_i2c *miot_i2c_get_global(void) {
  return s_global_i2c;
}

#endif /* MIOT_ENABLE_I2C */
