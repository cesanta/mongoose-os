// This example demonstrates how to subscribe to the MQTT topic and execute
// commands sent via MQTT messages.
//
// To try this example,
//   1. Download `mos` tool from https://mongoose-os.com/software.html
//   2. Run `mos` tool and install Mongoose OS
//   3. In the UI, navigate to the `Examples` tab and load this example

// Load Mongoose OS API
load('api_mqtt.js');

let topic = 'my/topic/#';
MQTT.sub(topic, function(conn, topic, msg) {
  print('Topic: ', topic, 'message:', msg);
}, null);

print('Subscribed to ', topic);
print('Use http://www.hivemq.com/demos/websocket-client to send messages');
