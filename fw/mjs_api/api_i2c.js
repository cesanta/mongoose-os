// I2C API. Source C API is defined at:
// [mgos_i2c.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_i2c.h)

let I2C = {
  _rrn: ffi('int mgos_i2c_read_reg_n(void *, int, int, int, char *)'),
  // ## **`I2C.get_default()`**
  // Get I2C handle. Return value: opaque pointer.
  get_default: ffi('void *mgos_i2c_get_global(void)'),

  // ## **`I2C.close()`**
  // Close I2C handle. Return value: none.
  close: ffi('void mgos_i2c_close(void *conn)'),

  // ## **`I2C.write(handle, addr, buf, size, stop)`**
  // Send a byte array to I2C.
  // If stop is true, the bus will be released at the end.
  // Return value: success, true/false.
  write: ffi('bool mgos_i2c_write(void *, int, char *, int, bool)'),

  // ## **`I2C.read(handle, addr, data, len, stop)`**
  // Read specified number of
  // bytes from the specified address.
  // If stop is true, the bus will be released at the end.
  // Return value: success, true/false.
  read: ffi('bool mgos_i2c_read(void *, int, char *, int, bool)'),

  // ## **`I2C.stop(handle)`**
  // Set i2c Stop condition. Releases the bus.
  // Return value: none.
  stop: ffi('void mgos_i2c_stop(void *)'),

  readRegB: ffi('int mgos_i2c_read_reg_b(void *, int, int)'),
  readRegW: ffi('int mgos_i2c_read_reg_w(void *, int, int)'),
  readRegN: function(i2c, addr, reg, num) {
    let buf = '';
    for (let i = 0; i < num; i++) buf += ' ';
    return (this._rrn(i2c, addr, reg, num, buf) === 1) ? buf : '';
  },

  writeRegB: ffi('bool mgos_i2c_write_reg_b(void *, int, int, int)'),
  writeRegW: ffi('bool mgos_i2c_write_reg_w(void *, int, int, int)'),
  writeRegN: ffi('int mgos_i2c_write_reg_n(void *, int, int, int, char *)'),
};
