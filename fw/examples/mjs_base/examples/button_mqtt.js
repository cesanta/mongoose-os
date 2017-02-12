// This example demonstrates how to react on a button press
// by sending a message to the MQTT topic.
//
// To try this example,
//   1. Download `mos` tool from https://mongoose-os.com/software.html
//   2. Run `mos` tool and install Mongoose OS
//   3. In the UI, navigate to the `Examples` tab and load this example

// Load Mongoose OS API
load('api_gpio.js');
load('api_mqtt.js');
load('api_sys.js');

let pin = 0;   // GPIO 0 is typically a 'Flash' button
GPIO.set_button_handler(pin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 50, function(x) {
  let topic = 'mOS/topic1';
  let message = JSON.stringify({
    total_ram: Sys.total_ram(),
    free_ram: Sys.free_ram()
  });
  let ok = MQTT.pub(topic, message, message.length);
  print('Published:', ok ? 'yes' : 'no', 'topic:', topic, 'message:', message);
}, true);

print('Flash button is configured on GPIO pin ', pin);
print('Press the flash button now!');
