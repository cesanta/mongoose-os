/*
 * MCP9898 temperature sensor JS I2C wrapper sample.
 * Example usage:
 *   var t = new MCP9808(new I2C(14, 12), MCP9808.addr(1, 1, 1));
 *   print(t.getTemp());
 */

function MCP9808(bus, addr) {
  this.getTemp = function() {
    // Read from the temperature register (5).
    var r = bus.do(addr, [I2C.WRITE, 5], [I2C.READ, 2]);
    if (r[r.length - 1] == I2C.ERR) {
      return -255;
    }
    var temp = (r[1].at(0) << 8) + r[1].at(1);
    var negative = (temp & (1 << 12)) != 0;
    temp &= 0xfff;  // leave only data bits.
    temp /= 16;     // normalize.
    if (negative) {
      temp -= 256;
    }
    return temp;
  }
}

MCP9808.addr = function(a2, a1, a0) {
  return 0b0011000 | (a2 << 2) | (a1 << 1) | a0;
}
