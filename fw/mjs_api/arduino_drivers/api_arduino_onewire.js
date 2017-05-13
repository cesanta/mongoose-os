// Arduino OneWire driver API. Source C API is defined at:
// [mgos_arduino_onewire.h](https://github.com/cesanta/mongoose-os/blob/master/arduino_drivers/Arduino/mgos_arduino_onewire.h)

let OneWire = {
  //Initialize 1-Wire bus. Return value: OneWire handle opaque pointer.
  init: ffi('void *mgos_arduino_onwire_init(int)'),
  // Close OneWire handle. Return value: none.
  close: ffi('void mgos_arduino_onwire_close(void *)'),
  reset: ffi('int mgos_arduino_onwire_reset(void *)'),
  select: ffi('void mgos_arduino_onwire_select(void *, char *)'),
  skip: ffi('void mgos_arduino_onwire_skip(void *)'),
  write: ffi('void mgos_arduino_onwire_write(void *, int)'),
  write_bytes: ffi('void mgos_arduino_onwire_write_bytes(void *, char *, int)'),
  read: ffi('int mgos_arduino_onwire_read(void *)'),
  read_bytes: ffi('void mgos_arduino_onwire_read_bytes(void *, char *, int)'),
  write_bit: ffi('void mgos_arduino_onwire_write_bit(void *, int)'),
  read_bit: ffi('int mgos_arduino_onwire_read_bit(void *)'),
  depower: ffi('void mgos_arduino_onwire_depower(void *)'),
  reset_search: ffi('void mgos_arduino_onwire_reset_search(void *)'),
  target_search: ffi('void mgos_arduino_onwire_target_search(void *, int)'),
  search: ffi('int mgos_arduino_onwire_search(void *, char *, int)'),
  crc8: ffi('int mgos_arduino_onwire_crc8(void *, char *, int)'),
  delay: ffi('void mgos_msleep(int)'),
};
