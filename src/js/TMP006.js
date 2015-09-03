/*
 * TMP006 temperature sensor JS I2C driver.
 * http://www.ti.com/product/TMP006/datasheet
 * Example usage:
 *   var i2c = new I2C();
 *   var t = new TMP006(i2c, 0x41);
 *   print(t.getTemp());
 *   i2c.close();
 */

function TMP006(bus, addr) {
  function toWord(s) {
    return (s.at(0) << 8) | s.at(1);
  }

  this.getTemp = function() {
    var r = bus.do(addr, [I2C.WRITE, 1], [I2C.READ, 2]);
    if (r[r.length - 1] == I2C.ERR) return undefined;
    var temp = toWord(r[1]) >> 2;
    if (temp & (1 << 13)) temp = -((1 << 14) - temp);
    return temp / 32;
  }

  // Read vendor and device ID.
  this.getID = function() {
    var r = bus.do(addr, [I2C.WRITE, 0xFE], [I2C.READ, 2], [I2C.WRITE, 0xFF], [I2C.READ, 2]);
    if (r[r.length - 1] == I2C.ERR) {
      return undefined;
    }
    return [toWord(r[1]), toWord(r[3])];
  }
}
