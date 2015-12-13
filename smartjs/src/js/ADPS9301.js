/*
 * Avago ADPS-9301 ambient light sensor driver.
 * Datasheet: http://goo.gl/ubKtKe
 */

// i2c - the I2C object.
// addr_sel - the state of the address selection pin:
// 0 - GND, 1 - Vdd, 2 - floating.
function ADPS9301(i2c, addr_sel) {
  var ADDR = (addr_sel == 0 ? 0x20 : (addr_sel == 1 ? 0x40 : 0x30)) | 9;

  // Turn sensor on/off. Turn on before use.
  this.setPower = function(on) {
    return i2c.writeRegB(ADDR, 0x80, on ? 3 : 0) == 0;
  }

  // Read sensor data from sensor 0 or 1.
  this.readData = function(sensor) {
    var reg = 0xAC | ((sensor & 1) << 1);
    return i2c.readRegW(ADDR, reg);
  }

  // Set options.
  //   hiGain - true/1 for high gain mode, false/0 for low.
  //   intgTm - integration time. 0 - 13.7 ms, 1 - 101 ms, 2 - 402 ms.
  this.setOpts = function(hiGain, intgTm) {
    return i2c.writeRegB(ADDR, 0x81, ((hiGain & 1) << 4) | (intgTm & 3)) == 0;
  }

  this.getID = function() {
    return i2c.readRegB(ADDR, 0x8A);
  }
}
