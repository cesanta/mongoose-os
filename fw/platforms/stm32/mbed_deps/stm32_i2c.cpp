#include "mbed.h"
#include "fw/src/miot_i2c.h"

struct miot_i2c {
} s_miot_i2c;

struct miot_i2c *miot_i2c_create(const struct sys_config_i2c *cfg) {
  /* TODO(alex): implement */
  (void) cfg;
  return &s_miot_i2c;
}

enum i2c_ack_type miot_i2c_start(struct miot_i2c *conn, uint16_t addr,
                                 enum i2c_rw mode) {
  /* TODO(alex): implement */
  (void) conn;
  (void) addr;
  (void) mode;

  return I2C_ERR;
}

void miot_i2c_stop(struct miot_i2c *conn) {
  /* TODO(alex): implement */

  (void) conn;
}

enum i2c_ack_type miot_i2c_send_byte(struct miot_i2c *conn, uint8_t data) {
  /* TODO(alex): implement */

  (void) conn;
  (void) data;

  return I2C_ERR;
}

uint8_t miot_i2c_read_byte(struct miot_i2c *conn, enum i2c_ack_type ack_type) {
  /* TODO(alex): implement */

  (void) conn;
  (void) ack_type;

  return 0;
}
