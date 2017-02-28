#include "fw/src/mgos_i2c.h"
#include "mcp9808.h"

#define MCP9808_ADDR 0x1F

double mcp9808_read_temp(void) {
  double ret = -1000;
  struct mgos_i2c *i2c = mgos_i2c_get_global();
  int v = mgos_i2c_read_reg_w(i2c, MCP9808_ADDR, 0x05);
  if (v >= 0) {
    ret = ((v >> 4) & 0xff) + (v & 0xf) / 16.0;
    if (v & 0x1000) ret = -ret;
  }
  return ret;
}
