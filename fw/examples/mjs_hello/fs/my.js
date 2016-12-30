let get_gpio = ffi("int get_gpio()");
let mgos_gpio_write = ffi("void mgos_gpio_write(int, int)");
let mgos_set_timer = ffi("int mgos_set_timer(int, int, void(*)(userdata), userdata)");

let level = 0;

mgos_set_timer(
  1000, // msecs
  1,    // repeat
  function(pin) {
    if (level == 1) {
      print("MJS Tick");
      level = 0;
    } else {
      print("MJS Tock");
      level = 1;
    }
    mgos_gpio_write(pin, level);
  },
  get_gpio()
);
