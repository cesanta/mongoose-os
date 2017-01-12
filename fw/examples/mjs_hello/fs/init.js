// Load Mongoose OS API
load('api_timer.js');
load('api_gpio.js');

// Blink built-in LED every second
let PIN = ffi('int get_led_gpio_pin()')();  // Helper C function that returns a
                                            // built-in LED GPIO
GPIO.set_mode(PIN, GPIO.MODE_OUTPUT);
Timer.set(1000 /* milliseconds */, 1 /* repeat */, function(pin) {
  let value = GPIO.toggle(pin);
  print(value ? 'Tick' : 'Tock');
}, PIN);
