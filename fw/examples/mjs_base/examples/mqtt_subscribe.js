// This example demonstrates how to subscribe to the MQTT topic and execute
// commands sent via MQTT messages.
//
// To try this example,
//   1. Download `mos` tool from https://mongoose-os.com/software.html
//   2. Run `mos` tool and install Mongoose OS
//   3. In the UI, navigate to the `Examples` tab and load this example

// Load Mongoose OS API
load('api_gpio.js');
load('api_mqtt.js');
load('api_sys.js');

let topic = 'test/mos1';
MQTT.sub(topic, function(conn, msg) {
  print('Got message: ', msg);
  let obj = JSON.parse(msg);
  print(JSON.stringify(obj));
  print('Setting GPIO pin', obj.pin, 'to', obj.state);
  GPIO.set_mode(obj.pin, GPIO.MODE_OUTPUT);
  GPIO.write(obj.pin, obj.state);
}, true);

print('Subscribed to ', topic);
print('Send me JSON commands, like {"pin": 2, "state": 0}');

// In order to send an MQTT message, use Web MQTT client at
// http://mqtt-helper.mybluemix.net , connect to broker.hivemq.com port 8000,
// and publish to topic 'test/mos1' message '{"pin": 2, "state": 0}'.
