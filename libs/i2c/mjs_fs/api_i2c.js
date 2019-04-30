let I2C = {
  _rrn: ffi('int mgos_i2c_read_reg_n(void *, int, int, int, char *)'),
  _r: ffi('bool mgos_i2c_read(void *, int, char *, int, bool)'),

  // ## **`I2C.get()`**
  // Get I2C bus handle. Return value: opaque pointer.
  get: ffi('void *mgos_i2c_get_global(void)'),
  get_default: ffi('void *mgos_i2c_get_global(void)'),  // deprecated

  // ## **`I2C.close(handle)`**
  // Close I2C handle. Return value: none.
  close: ffi('void mgos_i2c_close(void *)'),

  // ## **`I2C.write(handle, addr, buf, size, stop)`**
  // Send a byte array to I2C.
  // If stop is true, the bus will be released at the end.
  // Return value: success, true/false.
  write: ffi('bool mgos_i2c_write(void *, int, char *, int, bool)'),

  // ## **`I2C.read(handle, addr, len, stop)`**
  // Read specified number of bytes from the specified address.
  // If stop is true, the bus will be released at the end.
  // Return value: null on error, string with data on success. Example:
  // ```javascript
  // let data = I2C.read(bus, 31, 3, true);  // Read 3 bytes
  // if (data) print(JSON.stringify([data.at(0), data.at(1), data.at(2)]));
  // ```
  read: function(h, addr, len, stop) {
    let chunk = '          ', buf = chunk;
    while (buf.length < len) buf += chunk;
    let ok = this._r(h, addr, buf, len, stop);
    return ok ? buf.slice(0, len) : null;
  },

  // ## **`I2C.stop(handle)`**
  // Set i2c Stop condition. Releases the bus.
  // Return value: none.
  stop: ffi('void mgos_i2c_stop(void *)'),

  // ## **`I2C.readRegB(handle, addr, reg)`**
  // Read 1-byte register `reg` from the device at address `addr`; in case of
  // success return a numeric byte value from 0x00 to 0xff; otherwise return
  // -1. Example:
  // ```javascript
  // // Read 1 byte from the register 0x40 of the device at the address 0x12
  // let val = I2C.readRegB(bus, 0x12, 0x40);
  // if (val >= 0) print(val);
  // ```
  readRegB: ffi('int mgos_i2c_read_reg_b(void *, int, int)'),

  // ## **`I2C.readRegW(handle, addr, reg)`**
  // Read 2-byte register `reg` from the device at address `addr`; in case of
  // success return a numeric value; e.g. if 0x01, 0x02 was read from a device,
  // 0x0102 will be returned. In case of a failure return -1.
  // ```javascript
  // // Read 2 bytes from the register 0x40 of the device at the address 0x12
  // let val = I2C.readRegW(bus, 0x12, 0x40);
  // if (val >= 0) print(val);
  // ```
  readRegW: ffi('int mgos_i2c_read_reg_w(void *, int, int)'),

  // ## **`I2C.readRegN(handle, addr, reg, num)`**
  // Read N-byte register `reg` from the device at address `addr`. In case of
  // success return a string with data; otherwise return an empty string.
  //
  // E.g. if 0x61, 0x62, 0x63 was read from a device, "abc" will be returned.
  // You can get numeric values using `at(n)`, e.g. `"abc".at(0)` is `0x61`.
  //
  // ```javascript
  // // Read 7 bytes from the register 0x40 of the device at the address 0x12
  // let buf = I2C.readRegN(bus, 0x12, 0x40, 7);
  // if (buf != "") for (let i = 0; i < buf.length; i++) { print(buf.at(i)); }
  // ```
  readRegN: function(i2c, addr, reg, num) {
    let buf = '';
    for (let i = 0; i < num; i++) buf += ' ';
    return (this._rrn(i2c, addr, reg, num, buf) === 1) ? buf : '';
  },

  // ## **`I2C.writeRegB(handle, addr, reg, val)`**
  // Write numeric `val` (from 0x00 to 0xff) into 1-byte register `reg` at
  // address `addr`.  Return `true` on success, `false` on failure.
  // ```javascript
  // // Write a byte 0x55 to the register 0x40 of the device at the address 0x12
  // let result = I2C.writeRegB(bus, 0x12, 0x40, 0x55);
  // if (result) print('success') else print('failure');
  // ```
  writeRegB: ffi('bool mgos_i2c_write_reg_b(void *, int, int, int)'),

  // ## **`I2C.writeRegW(handle, addr, reg, val)`**
  // Write numeric `val` into 2-byte register `reg` at address `addr`. E.g.
  // if `val` is `0x0102`, then `0x01, 0x02` will be written.
  // Return `true` on success, `false` on failure.
  // ```javascript
  // // Write a [0x55, 0x66] to the register 0x40 of the device at the address 0x12
  // let result = I2C.writeRegW(bus, 0x12, 0x40, 0x5566);
  // if (result) print('success') else print('failure');
  // ```
  writeRegW: ffi('bool mgos_i2c_write_reg_w(void *, int, int, int)'),

  // ## **`I2C.writeRegN(handle, addr, reg, n, buf)`**
  // Write n first bytes of the string `buf` into the  register `reg` at
  // address `addr`. E.g.  if `buf` is `"abc"`, then `0x61, 0x62, 0x63` will be
  // written.
  // Return `true` on success, `false` on failure.
  // ```javascript
  // // Write a [0x55, 0x66, 0x77] to the register 0x40 of the device at the address 0x12
  // let result = I2C.writeRegN(bus, 0x12, 0x40, 3, "\x55\x66\x77");
  // if (result) print('success') else print('failure');
  // ```
  writeRegN: ffi('int mgos_i2c_write_reg_n(void *, int, int, int, char *)'),
};
