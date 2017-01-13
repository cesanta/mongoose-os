// This example demonstrates how to react on a button press
// by printing a message on a console.

// Load Mongoose OS API
load('api_gpio.js');

// Configure button GPIO
// Button is attached to GPIO 0, a flash button
let button = 0;
GPIO.set_mode(button, GPIO.MODE_INPUT);
GPIO.set_pull(button, GPIO.PULL_UP);

// Configure LED GPIO
let led = ffi('int get_led_gpio_pin()')();  // Get built-in LED GPIO pin
GPIO.set_mode(led, GPIO.MODE_OUTPUT);

// Set interrupt handler
GPIO.set_int_handler(button, GPIO.INT_EDGE_NEG, function(x) {
  let value = GPIO.toggle(led);
  print('Button press, pin:', x, 'LED pin:', led, ' Value: ', value);
}, true);

// Enable interrupts. Must be called last, after GPIO.set_int_handler().
GPIO.enable_int(button);
