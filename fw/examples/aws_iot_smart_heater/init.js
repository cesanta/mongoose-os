// This example demonstrates how to react on a button press
// by sending a message to AWS IoT.
//
// See README.md for details.
//
// Load Mongoose OS API
load('api_timer.js');
load('api_mqtt.js');
load('api_config.js');

let devID = Cfg.get('device.id');
let topic = devID + '/temp';
let tempDummy = 10;
Timer.set(5000 /* milliseconds */, 1 /* repeat */, function() {
  let message = JSON.stringify({
    temp: tempDummy,
  });
  let ok = MQTT.pub(topic, message, message.length);
  print('Published:', ok ? 'yes' : 'no', 'topic:', topic, 'message:', message);

  tempDummy++;
  if (tempDummy > 30) {
    tempDummy = 10;
  }
}, null);
