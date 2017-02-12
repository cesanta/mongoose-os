// This example demonstrates how to react on a button press by toggling LED.
//
// To try this example,
//   1. Download `mos` tool from https://mongoose-os.com/software.html
//   2. Run `mos` tool and install Mongoose OS
//   3. In the UI, navigate to the `Examples` tab and load this example

// Load Mongoose OS API
load('api_gpio.js');

// Configure LED
let led = ffi('int get_led_gpio_pin()')();  // Get built-in LED GPIO pin
GPIO.set_mode(led, GPIO.MODE_OUTPUT);

let pin = 0;   // GPIO 0 is typically a 'Flash' button
GPIO.set_button_handler(pin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 50, function(x) {
  let value = GPIO.toggle(led);
  print('Button press, pin:', x, 'LED pin:', led, ' Value: ', value);
}, true);

print('Flash button is configured on GPIO pin ', pin);
print('Press the flash button now!');
