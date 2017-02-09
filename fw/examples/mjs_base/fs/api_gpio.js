// GPIO API. Source C API is defined at:
// https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_gpio.h

let GPIO = {
  toggle: ffi('int mgos_gpio_toggle(int)'),
  write: ffi('void mgos_gpio_write(int,int)'),
  read: ffi('int mgos_gpio_read(int)'),

  set_mode: ffi('int mgos_gpio_set_mode(int,int)'),
  MODE_INPUT: 0,
  MODE_OUTPUT: 1,

  set_pull: ffi('int mgos_gpio_set_pull(int,int)'),
  PULL_NONE: 0,
  PULL_UP: 1,
  PULL_DOWN: 2,

  enable_int: ffi('int mgos_gpio_enable_int(int)'),
  disable_int: ffi('int mgos_gpio_disable_int(int)'),
  set_int_handler: ffi(
      'int mgos_gpio_set_int_handler(int,int,void(*)(int,userdata),userdata)'),
  set_button_handler: ffi('int mgos_gpio_set_button_handler(int,int,int,int,void(*)(int, userdata), userdata)'),
  INT_NONE: 0,
  INT_EDGE_POS: 1,
  INT_EDGE_NEG: 2,
  INT_EDGE_ANY: 3,
  INT_LEVEL_HI: 4,
  INT_LEVEL_LO: 5
};
