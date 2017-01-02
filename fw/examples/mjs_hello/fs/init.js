let gpio_set_mode = ffi('void mgos_gpio_set_mode(int,int)');
let gpio_toggle = ffi('void mgos_gpio_toggle(int)');
let gpio_read = ffi('void mgos_gpio_read(int)');
let timer_set = ffi('int mgos_set_timer(int,int,void(*)(userdata),userdata)');
let get_pin = ffi('int get_led_gpio_pin()');


// On NodeMCU, pin 2 is a built-in blue LED
// On CC3200 Launchpad, use pin 64 (a built-in red LED)
let PIN = get_pin();
let callback = function(pin) {
  gpio_toggle(pin);
  if (gpio_read(pin)) {
    print('Tick');
  } else {
    print('Tock');
  }
};

gpio_set_mode(PIN, 1);
timer_set(500 /* milliseconds */, 1 /* repeat */, callback, PIN);
