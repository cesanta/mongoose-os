#include "fw/src/miot_i2c.h"
#include "mcp9808.h"

#define MCP9808_ADDR 0x1F

double mc6808_read_temp(void) {
  double ret = -1000;
  struct miot_i2c *i2c = miot_i2c_get_global();
  if (miot_i2c_start(i2c, MCP9808_ADDR, I2C_WRITE) != I2C_ACK) {
    return -1000;
  }
  miot_i2c_send_byte(i2c, 0x05);
  if (miot_i2c_start(i2c, MCP9808_ADDR, I2C_READ) != I2C_ACK) {
    return -1000;
  }
  uint8_t upper_byte = miot_i2c_read_byte(i2c, I2C_ACK);
  uint8_t lower_byte = miot_i2c_read_byte(i2c, I2C_NAK);
  miot_i2c_stop(i2c);
  upper_byte &= 0x1f;
  if (upper_byte & 0x10) {
    upper_byte &= 0xf;
    ret = -(256 - (upper_byte * 16.0 + lower_byte / 16.0));
  } else {
    ret = (upper_byte * 16.0 + lower_byte / 16.0);
  }
  return ret;
}
