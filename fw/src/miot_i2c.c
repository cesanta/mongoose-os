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

enum i2c_ack_type miot_i2c_send_bytes(struct miot_i2c *c, uint8_t *buf,
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
