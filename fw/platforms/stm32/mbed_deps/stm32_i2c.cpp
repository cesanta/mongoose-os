#include "mbed.h"
#include "fw/src/mgos_i2c.h"

struct mgos_i2c {
} s_mgos_i2c;

struct mgos_i2c *mgos_i2c_create(const struct sys_config_i2c *cfg) {
  /* TODO(alex): implement */
  (void) cfg;
  return &s_mgos_i2c;
}

enum i2c_ack_type mgos_i2c_start(struct mgos_i2c *conn, uint16_t addr,
                                 enum i2c_rw mode) {
  /* TODO(alex): implement */
  (void) conn;
  (void) addr;
  (void) mode;

  return I2C_ERR;
}

void mgos_i2c_stop(struct mgos_i2c *conn) {
  /* TODO(alex): implement */

  (void) conn;
}

enum i2c_ack_type mgos_i2c_send_byte(struct mgos_i2c *conn, uint8_t data) {
  /* TODO(alex): implement */

  (void) conn;
  (void) data;

  return I2C_ERR;
}

uint8_t mgos_i2c_read_byte(struct mgos_i2c *conn, enum i2c_ack_type ack_type) {
  /* TODO(alex): implement */

  (void) conn;
  (void) ack_type;

  return 0;
}
