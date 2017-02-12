// This example demonstrates how to send a metric (free RAM) to ThingSpeak.
//
// To try this example,
//   1. Download `mos` tool from https://mongoose-os.com/software.html
//   2. Run `mos` tool and install Mongoose OS
//   3. In the UI, navigate to the `Examples` tab and load this example

// Load Mongoose OS API
load('api_mqtt.js');
load('api_sys.js');
load('api_timer.js');

let topic = 'channels/221046/publish/fields/field1/T9N0OAI9CH7M1W4W';
let freq = 5000; // Milliseconds. Report values that often.

Timer.set(freq, 1, function() {
  let msg = JSON.stringify(Sys.free_ram());
  MQTT.pub(topic, msg, msg.length);
}, true);
