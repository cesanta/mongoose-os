// This example demonstrates how to react on a button press
// by printing a message on a console.

// Load Mongoose OS API
load('api_gpio.js');

// Button is attached to GPIO 0, a flash button
let pin = 0;

// Configure button GPIO
GPIO.set_mode(pin, GPIO.MODE_INPUT);
GPIO.set_pull(pin, GPIO.PULL_UP);

// Set interrupt handler
GPIO.set_int_handler(pin, GPIO.INT_EDGE_NEG, function(x) {
  print('Button press, pin: ', x);
}, true);

// Enable interrupts. Must be called last, after GPIO.set_int_handler().
GPIO.enable_int(pin);
