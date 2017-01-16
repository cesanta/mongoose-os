// This example demonstrates how to react on a button press
// by printing a message on a console.

// Load Mongoose OS API
load('api_gpio.js');

let pin = 0;   // GPIO 0 is typically a 'Flash' button
GPIO.set_button_handler(pin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 50, function(x) {
  print('Button press, pin: ', x);
}, true);
