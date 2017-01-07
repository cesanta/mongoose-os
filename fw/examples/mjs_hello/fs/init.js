let GPIO = {
  set_mode: ffi('void mgos_gpio_set_mode(int,int)'),
  toggle: ffi('void mgos_gpio_toggle(int)'),
  read: ffi('void mgos_gpio_read(int)')
};

let Timer = {
  set: ffi('int mgos_set_timer(int,int,void(*)(userdata),userdata)')
};

let callback = function(pin) {
  GPIO.toggle(pin);
  print(GPIO.read(pin) ? 'Tick' : 'Tock');
};

// On NodeMCU, pin 2 is a built-in blue LED
// On CC3200 Launchpad, use pin 64 (a built-in red LED)
let PIN = ffi('int get_led_gpio_pin()')();

GPIO.set_mode(PIN, 1);
Timer.set(1000 /* milliseconds */, 1 /* repeat */, callback, PIN);
