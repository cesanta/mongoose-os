// This example demonstrates how to create a TCP echo server.
//
// To try this example,
//   1. Download `mos` tool from https://mongoose-os.com/software.html
//   2. Run `mos` tool and install Mongoose OS
//   3. In the UI, navigate to the `Examples` tab and load this example

// Load Mongoose OS API
load('api_gpio.js');
load('api_i2c.js');
load('api_mqtt.js');
load('api_rpc.js');
load('api_sys.js');
load('api_timer.js');
load('api_config.js');

// GPIO pin which has a on/off relay connected
let pin = 13;
GPIO.set_mode(pin, GPIO.MODE_OUTPUT);

// Milliseconds. How often to send temperature readings to the cloud
let freq = 10000;

// MQTT topic to publish to. MQTT server is configured separately,
// mos config-set mqtt.server=YOUR_SERVER:PORT
let devID = Cfg.get('device.id');
let topic = devID + '/temp';

// This function reads temperature from the MCP9808 temperature sensor.
// Data sheet: http://www.microchip.com/wwwproducts/en/en556182
let getTemp = function() {
  let i2c = I2C.get_default();
  let t = -1000;
  let v = I2C.readRegW(i2c, 0x1f, 5);
  if (v > 0) {
    t = ((v >> 4) & 0xff) + ((v & 0xf) / 16.0);
    if (v & 0x1000) t = -t;
  }
  return t;
};

let getStatus = function() {
  return {
    temp: getTemp(),
    on: GPIO.read(pin) === 1
  };
};

RPC.addHandler('Heater.SetState', function(args) {
  GPIO.write(pin, args.on || 0);
  return true;
});

RPC.addHandler('Heater.GetState', function(args) {
  return getStatus();
});

// Send temperature readings to the cloud
Timer.set(freq, true, function() {
  let message = JSON.stringify(getStatus());
  let ok = MQTT.pub(topic, message, 1);
  print('MQTT pubish: topic ', topic, 'msg: ', message, 'status: ', ok);
}, null);
