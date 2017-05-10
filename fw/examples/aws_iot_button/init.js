// This example demonstrates how to react on a button press
// by sending a message to AWS IoT.
//
// See README.md for details.
//
// Load Mongoose OS API
load('api_gpio.js');
load('api_mqtt.js');
load('api_sys.js');
load('api_config.js');

let pin = 0;   // GPIO 0 is typically a 'Flash' button
GPIO.set_button_handler(pin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 50, function(x) {
  let topic = Cfg.get('device.id') + '/button_pressed';
  let message = JSON.stringify({
    total_ram: Sys.total_ram(),
    free_ram: Sys.free_ram()
  });
  let ok = MQTT.pub(topic, message, 1);
  print('Published:', ok ? 'yes' : 'no', 'topic:', topic, 'message:', message);
}, true);

print('Flash button is configured on GPIO pin ', pin);
print('Press the flash button now!');
