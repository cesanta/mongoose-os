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

// GPIO pin which has a on/off relay connected
let pin = 13;
GPIO.set_mode(pin, GPIO.MODE_OUTPUT);

// Milliseconds. How often to send temperature readings to the cloud
let freq = 10000;

// MQTT topic to publish to. MQTT server is configured separately,
// mos config-set mqtt.server=YOUR_SERVER:PORT
let topic = 'topic1';

// This function reads temperature from the MCP9808 temperature sensor.
// Data sheet: http://www.microchip.com/wwwproducts/en/en556182
let getTemp = function() {
  let i2c = I2C.get_default();
  I2C.start(i2c, 0x1f, I2C.WRITE);
  I2C.write(i2c, '\x05');
  I2C.start(i2c, 0x1f, I2C.READ);
  let hi = I2C.read(i2c, 1);
  let lo = I2C.read(i2c, 1);
  I2C.stop(i2c);
  let temp = -1;
  hi &= 0x1f;
  if (hi & 0x10) {
    hi &= 0xf;
    temp = -(256 - (hi * 16.0 + lo / 16.0));
  } else {
    temp = (hi * 16.0 + lo / 16.0);
  }
  return temp;
};

let getStatus = function() {
  return {
    temp: getTemp(),
    on: GPIO.read(pin)
  };
};

RPC.addHandler('Heater.SetState', function(args) {
  GPIO.write(pin, args.state || 0);
  return true;
});

RPC.addHandler('Heater.GetState', function(args) {
  return getStatus();
});

// Send temperature readings to the cloud
Timer.set(freq, 1, function() {
  let message = JSON.stringify(getStatus());
  let ok = MQTT.pub(topic, message, message.length);
  print('MQTT pubish: topic ', topic, 'msg: ', message, 'status: ', ok);
}, null);
