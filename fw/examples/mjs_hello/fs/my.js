get_gpio = ffi("int get_gpio()");
miot_gpio_write = ffi("void miot_gpio_write(int, int)");
miot_set_timer = ffi("int miot_set_timer(int, int, void*, void*)");

level = 0;

miot_set_timer(
  1000, // msecs
  1,    // repeat
  ffi_cb_void_void(), // returns a C callback which returns nothing and takes
                      // also nothing (other than the user_data pointer)
  ffi_cb_arg(function(pin) {
    if (level == 1) {
      print("MJS Tick");
      level = 0;
    } else {
      print("MJS Tock");
      level = 1;
    }
    miot_gpio_write(pin, level);

  }, get_gpio())
);
