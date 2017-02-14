// I2C API. Source C API is defined at:
// [mgos_i2c.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_i2c.h)

let I2C = {
  // **`I2C.get_default()`** - get I2C handle. Return value: opaque pointer.
  get_default: ffi('void *mgos_i2c_get_global(void)'),

  // **`I2C.close()`** - close I2C handle. Return value: none.
  close: ffi('void mgos_i2c_close(void *conn)'),

  // **`I2C.start(handle, addr, mode)`** - start I2C transaction.
  // `addr` is an address on the I2C bus, from 0 to 255.
  // `mode` is either `I2C.WRITE` or `I2C.READ`.
  // Return value: one of the following: `I2C.ACK`, `I2C.NAK`, `I2C.ERR`, `I2C.NONE`
  start: ffi('int mgos_i2c_start(void *, int, int)'),
  // **`I2C.stop(handle)`** - Set i2c Stop condition. Releases the bus.
  // Return value: none.
  stop: ffi('void mgos_i2c_stop(void *)'),
  // **`I2C.write(handle, buf, size)`** - Send a byte array to I2C.
  // Return value: ack type sent in response to the last transmitted byte, one
  // of the following: `I2C.ACK`, `I2C.NAK`, `I2C.ERR`, `I2C.NONE`.
  // Receiver must positively acknowledge all bytes except, maybe, the last one.
  // If all the bytes have been sent, the return value is the acknowledgement
  // status of the last one (`I2C.ACK` or `I2C.NAK`). If a `I2C.NAK` was
  // received before all the bytes could be sent, `I2C.ERR` is returned
  // instead.
  write: ffi('int mgos_i2c_send_bytes(void *, char *, int)'),
  // **`I2C.read(handle, ack_type)`** - Read one byte from the bus, finish with
  // an ack of the specified type (either `I2C.ACK` or `I2C.NAK`), or without
  // ack at all (if ack_type is `I2C.NONE`), in which case this call must be
  // followed by the `I2C.ack()`.
  // Return value: read byte, a number from 0 to 255.
  read: ffi('int mgos_i2c_read_byte(void *, int)'),
  // **`I2C.ack(handle, ack_type)`** - Send an ack of the specified type. Meant
  // to be used after `I2C.read()` with ack_type `I2C.NONE`.
  // Return value: none.
  ack: ffi('void mgos_i2c_send_ack(void *, int)'),
  ACK: 0,
  NAK: 1,
  ERR: 2,
  NONE: 3,
  WRITE: 0,
  READ: 1,
};
