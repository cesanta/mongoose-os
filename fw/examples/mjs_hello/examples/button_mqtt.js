// This example demonstrates how to react on a button press
// by sending a message to the MQTT topic.

// Load Mongoose OS API
load('api_gpio.js');
load('api_mqtt.js');
load('api_sys.js');

// Configure button GPIO
// Button is attached to GPIO 0, a flash button
let button = 0;
GPIO.set_mode(button, GPIO.MODE_INPUT);
GPIO.set_pull(button, GPIO.PULL_UP);

// Set interrupt handler
GPIO.set_int_handler(button, GPIO.INT_EDGE_NEG, function(x) {
  let topic = 'mOS/topic1';
  let message = JSON.stringify({
    total_ram: Sys.total_ram(),
    free_ram: Sys.free_ram()
  });
  let ok = MQTT.pub(topic, message, message.length);
  print('Published:', ok ? 'yes' : 'no', 'topic:', topic, 'message:', message);
}, true);

// Enable interrupts. Must be called last, after GPIO.set_int_handler().
GPIO.enable_int(button);
