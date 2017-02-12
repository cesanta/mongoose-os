// I2C API. Source C API is defined at:
// [mgos_i2c.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_i2c.h)

let I2C = {
  // **`I2C.get_default()`** - get I2C handle. Return value: opaque pointer.
  get_default: ffi('void *mgos_i2c_get_global(void)'),

  // **`I2C.close()`** - close I2C handle. Return value: none.
  close: ffi('void mgos_i2c_close(void *conn)'),

  // **`I2C.start(handle, addr, )`** - start I2C transaction.
  start: ffi('int mgos_i2c_start(void *, int, int)'),
  stop: ffi('void mgos_i2c_stop(void *)'),
  write: ffi('int mgos_i2c_send_bytes(void *, char *, int)'),
  read: ffi('int mgos_i2c_read_byte(void *, int)'),
  ack: ffi('void mgos_i2c_send_ack(void *, int)'),
  ACK: 0,
  NAK: 1,
  ERR: 2,
  NONE: 3,
  WRITE: 0,
  READ: 1,
};
