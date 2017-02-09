// System API. Source C API is defined at:
// https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_hal.h

let Sys = {
  peek: ffi('int mgos_peek(void *, int)'),
  total_ram: ffi('int mgos_get_heap_size()'),
  free_ram: ffi('int mgos_get_free_heap_size()'),
  reboot: ffi('void mgos_system_restart(int)'),
  usleep: ffi('void mgos_usleep(int)')
};
