// System API. Source C API is defined at:
// [mgos_hal.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_hal.h)

let Sys = {
  // **`Sys.peek(ptr, offset)`** - return byte value (a number) for given
  // pointer at offset.
  peek: ffi('int mgos_peek(void *, int)'),

  // **`Sys.total_ram()`** - return total available RAM in bytes.
  total_ram: ffi('int mgos_get_heap_size()'),

  // **`Sys.free_ram()`** - return free available RAM in bytes.
  free_ram: ffi('int mgos_get_free_heap_size()'),

  // **`Sys.reboot()`** - reboot the system.
  // Return value: none.
  reboot: ffi('void mgos_system_restart(int)'),

  // **`Sys.usleep(microseconds)`** - sleep given number of microseconds.
  // Return value: none.
  usleep: ffi('void mgos_usleep(int)')
};
