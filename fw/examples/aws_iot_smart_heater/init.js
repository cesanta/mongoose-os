// This example demonstrates how to react on a button press
// by sending a message to AWS IoT.
//
// See README.md for details.
//
// Load Mongoose OS API
load('api_timer.js');
load('api_mqtt.js');
load('api_config.js');
load('api_rpc.js');

let devID = Cfg.get('device.id');
let topic = devID + '/temp';
let tempDummy = 10;
let heaterOn = false;
Timer.set(1000 /* milliseconds */, 1 /* repeat */, function() {
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

let getTemp = function() {
  return tempDummy;
};

let getStatus = function() {
  return {
    temp: getTemp(),
    on: heaterOn
  };
};

RPC.addHandler('Heater.SetState', function(args) {
  //GPIO.write(pin, args.state || 0);
  print('setstate', JSON.stringify(args));
  heaterOn = args.on;
  return true;
});

RPC.addHandler('Heater.GetState', function(args) {
  return getStatus();
});

print('hey!');
