/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/mgos_i2c.h"

#if MGOS_ENABLE_I2C

static struct mgos_i2c *s_global_i2c;

int mgos_i2c_read_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg) {
  uint8_t value;
  if (!mgos_i2c_read_reg_n(conn, addr, reg, 1, &value)) {
    return -1;
  }
  return value;
}

int mgos_i2c_read_reg_w(struct mgos_i2c *conn, uint16_t addr, uint8_t reg) {
  uint8_t tmp[2];
  if (!mgos_i2c_read_reg_n(conn, addr, reg, 2, tmp)) {
    return -1;
  }
  return (((uint16_t) tmp[0]) << 8) | tmp[1];
}

bool mgos_i2c_read_reg_n(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                         size_t n, uint8_t *buf) {
  return mgos_i2c_write(conn, addr, &reg, 1, false /* stop */) &&
         mgos_i2c_read(conn, addr, buf, n, true /* stop */);
}

bool mgos_i2c_write_reg_b(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          uint8_t value) {
  uint8_t tmp[2] = {reg, value};
  return mgos_i2c_write(conn, addr, tmp, sizeof(tmp), true /* stop */);
}

bool mgos_i2c_write_reg_w(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          uint16_t value) {
  uint8_t tmp[3] = {reg, (uint8_t)(value >> 8), (uint8_t) value};
  return mgos_i2c_write(conn, addr, tmp, sizeof(tmp), true /* stop */);
}

bool mgos_i2c_write_reg_n(struct mgos_i2c *conn, uint16_t addr, uint8_t reg,
                          size_t n, const uint8_t *buf) {
  bool res = false;
  uint8_t *tmp = calloc(n + 1, 1);
  if (tmp) {
    *tmp = reg;
    memcpy(tmp + 1, buf, n);
    res = mgos_i2c_write(conn, addr, tmp, n + 1, true /* stop */);
    free(tmp);
  }
  return res;
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
