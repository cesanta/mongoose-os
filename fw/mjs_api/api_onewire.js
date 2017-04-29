// 1-Wire API. Source C API is defined at:
// [mgos_onewire.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_onewire.h)

let OneWire = {
  //Initialize 1-Wire bus. Return value: OneWire handle opaque pointer.
  init: ffi('void *mgos_onewire_init(int)'),
  // Close OneWire handle. Return value: none.
  close: ffi('void mgos_onewire_close(void *)'),
  reset: ffi('int mgos_onewire_reset(void *)'),
  targetSetup: ffi('void mgos_onewire_target_setup(void *, int)'),
  searchClean: ffi('void mgos_onewire_search_clean(void *)'),
  next: ffi('int mgos_onewire_next(void *, char *, int)'),
  select: ffi('void mgos_onewire_select(void *, char *)'),
  read: ffi('int mgos_onewire_read(void *)'),
  write: ffi('void mgos_onewire_write(void *, int)'),
  crc8: ffi('int mgos_onewire_crc8(char *, int)'),
  delay: ffi('void mgos_msleep(int)'),
};
