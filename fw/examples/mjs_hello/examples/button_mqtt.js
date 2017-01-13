// This example demonstrates how to react on a button press
// by sending a message to the MQTT topic.

// Load Mongoose OS API
load('api_gpio.js');
load('api_mqtt.js');

// Configure button GPIO
// Button is attached to GPIO 0, a flash button
let button = 0;
GPIO.set_mode(button, GPIO.MODE_INPUT);
GPIO.set_pull(button, GPIO.PULL_UP);

// Set interrupt handler
GPIO.set_int_handler(button, GPIO.INT_EDGE_NEG, function(x) {
  let topic = 'mOS/topic1';
  let message = 'hi!';
  let ok = MQTT.pub(topic, message, 3);
  print('Published:', ok ? 'yes' : 'no', 'topic:', topic, 'message:', message);
}, true);

// Enable interrupts. Must be called last, after GPIO.set_int_handler().
GPIO.enable_int(button);
