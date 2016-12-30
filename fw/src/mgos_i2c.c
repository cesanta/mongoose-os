/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_i2c.h"

#if MGOS_ENABLE_I2C

static struct mgos_i2c *s_global_i2c;

void mgos_i2c_read_bytes(struct mgos_i2c *c, size_t n, uint8_t *buf,
                         enum i2c_ack_type last_ack_type) {
  size_t i;

  for (i = 0; i < n; i++) {
    enum i2c_ack_type ack_type = (i != n - 1 ? I2C_ACK : last_ack_type);
    *buf++ = mgos_i2c_read_byte(c, ack_type);
  }
}

enum i2c_ack_type mgos_i2c_send_bytes(struct mgos_i2c *c, const uint8_t *buf,
                                      size_t buf_size) {
  enum i2c_ack_type ack_type = I2C_NAK;

  while (buf_size-- > 0) {
    ack_type = mgos_i2c_send_byte(c, *buf++);
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

static bool mgos_i2c_write_reg_addr(struct mgos_i2c *conn, uint16_t addr,
                                    uint8_t reg) {
  bool res = false;
  bool started = false;
  if (mgos_i2c_start(conn, addr, I2C_WRITE) != I2C_ACK) goto out;
  started = true;
  res = (mgos_i2c_send_byte(conn, reg) == I2C_ACK);
out:
  if (started && !res) mgos_i2c_stop(conn);
  return res;
}

static bool mgos_i2c_read_reg_n(struct mgos_i2c *conn, uint16_t addr,
                                uint8_t reg, size_t len, uint8_t *buf) {
  if (!mgos_i2c_write_reg_addr(conn, addr, reg) ||
      (mgos_i2c_start(conn, addr, I2C_READ) != I2C_ACK)) {
    return false;
  }
  mgos_i2c_read_bytes(conn, len, buf, I2C_ACK);
  mgos_i2c_stop(conn);
  return true;
}

bool mgos_i2c_read_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                         uint8_t *value) {
  return mgos_i2c_read_reg_n(conn, addr, reg, 1, value);
}

bool mgos_i2c_read_reg_w(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                         uint16_t *value) {
  uint8_t tmp[2];
  bool res = mgos_i2c_read_reg_n(conn, addr, reg, 2, tmp);
  if (res) {
    *value = (((uint16_t) tmp[0]) << 8) | tmp[1];
  } else {
    *value = 0;
  }
  return res;
}

static bool mgos_i2c_write_reg_n(struct mgos_i2c *conn, uint16_t addr,
                                 uint8_t reg, size_t len, const uint8_t *buf) {
  bool res = false;
  if (!mgos_i2c_write_reg_addr(conn, addr, reg)) return false;
  res = (mgos_i2c_send_bytes(conn, buf, len) == I2C_ACK);
  mgos_i2c_stop(conn);
  return res;
}

bool mgos_i2c_write_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          uint8_t value) {
  return mgos_i2c_write_reg_n(conn, addr, reg, 1, &value);
}

bool mgos_i2c_write_reg_w(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          uint16_t value) {
  uint8_t tmp[2] = {(uint8_t)(value >> 8), (uint8_t) value};
  return mgos_i2c_write_reg_n(conn, addr, reg, 2, tmp);
}

enum mgos_init_result mgos_i2c_init(void) {
  const struct sys_config_i2c *cfg = &get_cfg()->i2c;
  if (!cfg->enable) return MGOS_INIT_OK;
  s_global_i2c = mgos_i2c_create(cfg);
  return (s_global_i2c != NULL ? MGOS_INIT_OK : MGOS_INIT_I2C_FAILED);
}

struct mgos_i2c *mgos_i2c_get_global(void) {
  return s_global_i2c;
}

#endif /* MGOS_ENABLE_I2C */
