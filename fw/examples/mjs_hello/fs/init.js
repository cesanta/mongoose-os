// Load Mongoose OS API
exec_file('sys.js');

// Blink built-in LED every second
let PIN = ffi('int get_led_gpio_pin()')();  // Helper C function that returns a built-in LED GPIO
GPIO.set_mode(PIN, GPIO.MODE_OUTPUT);
Timer.set(1000 /* milliseconds */, 1 /* repeat */, function(pin) {
  GPIO.toggle(pin);
  print(GPIO.read(pin) ? 'Tick' : 'Tock');
}, PIN);

// Print a message on a button press
let pin2 = 0;
GPIO.set_mode(pin2, GPIO.MODE_INPUT);
GPIO.set_pull(pin2, GPIO.PULL_UP);
GPIO.set_int_handler(pin2, GPIO.INT_EDGE_NEG, function(pin) {
  print('Button press, pin: ', pin);
}, true);
GPIO.enable_int(pin2);  // Must be called after GPIO.set_int_handler()
