/*
 * MCP9898 temperature sensor JS I2C wrapper sample.
 * Example usage:
 *   var t = new MCP9808(14, 12, 1, 1); print(t.getTemp()); t.close();
 */

function MCP9808(sda, scl, a2, a1, a0) {
  var c = new I2C(sda, scl);
  var addr = 0b0011000 | (a2 << 2) | (a1 << 1) | a0;

  this.getTemp = function() {
    // Select the temperature register (5).
    if (c.start(addr, I2C.WRITE) != I2C.ACK) {
      return -255;
    }
    if (c.send(5) != I2C.ACK) {
      c.stop();
      return -255;
    }

    // Read its value.
    if (c.start(addr, I2C.READ) != I2C.ACK) {
      c.stop();
      return -255;
    }

    var temp_u, temp_l;

    temp_u = c.readByte(I2C.ACK);
    temp_l = c.readByte(I2C.NAK);

    c.stop();

    temp_u &= 31;

    if ((temp_u & 16) != 0) {
      temp_u &= 15;
      return -(256-(temp_u*16 + temp_l/16));
    } else {
      return temp_u*16 + temp_l/16;
    }
  }
}
