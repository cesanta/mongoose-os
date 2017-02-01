#include <stm32_sdk_hal.h>
#include "fw/src/mgos_i2c.h"

struct mgos_i2c {
  I2C_HandleTypeDef *hi2c;
};

struct mgos_i2c *mgos_i2c_create(const struct sys_config_i2c *cfg) {
  if (cfg->enable) {
    struct mgos_i2c *i2c = calloc(1, sizeof(*i2c));
    i2c->hi2c = &I2C_DEFAULT;
    return i2c;
  } else {
    return NULL;
  }
}

enum i2c_ack_type mgos_i2c_start(struct mgos_i2c *conn, uint16_t addr,
                                 enum i2c_rw mode) {
  /* TODO(alashkin): implement */
  (void) conn;
  (void) addr;
  (void) mode;
  return I2C_ERR;
}

void mgos_i2c_stop(struct mgos_i2c *conn) {
  /* TODO(alashkin): implement */
  (void) conn;
}

enum i2c_ack_type mgos_i2c_send_byte(struct mgos_i2c *conn, uint8_t data) {
  /* TODO(alashkin): implement */
  (void) conn;
  (void) data;
  return I2C_ERR;
}

uint8_t mgos_i2c_read_byte(struct mgos_i2c *conn, enum i2c_ack_type ack_type) {
  /* TODO(alashkin): implement */
  (void) conn;
  (void) ack_type;
  return 0xFF;
}

void mgos_i2c_close(struct mgos_i2c *conn) {
  free(conn);
}
