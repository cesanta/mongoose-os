// This example demonstrates how to create a TCP echo server.
//
// To try this example,
//   1. Download `mos` tool from https://mongoose-iot.com/software.html
//   2. Run `mos` tool and install Mongoose OS
//   3. In the UI, navigate to the `Examples` tab and load this example

// Load Mongoose OS API
load('api_gpio.js');
load('api_net.js');
load('api_http.js');
load('api_timer.js');
load('api_sys.js');
load('api_mqtt.js');

let listener = HTTP.get_system_server();
let pin = 13;
let redir = 'HTTP/1.0 302 OK\r\nLocation: /\r\n\r\n';
let freq = 10000; // Milliseconds. How often to send temperature readings

GPIO.set_mode(pin, GPIO.MODE_OUTPUT);

HTTP.add_endpoint(listener, '/heater/status', function(conn, ev, msg) {
  Net.send(conn, 'HTTP/1.0 200 OK\r\n\r\n');
  Net.send(conn, JSON.stringify({
    on: GPIO.read(pin)
  }));
  Net.disconnect(conn);
}, true);

HTTP.add_endpoint(listener, '/heater/on', function(conn, ev, msg) {
  GPIO.write(pin, 1);
  Net.send(conn, redir);
  Net.disconnect(conn);
}, true);

HTTP.add_endpoint(listener, '/heater/off', function(conn, ev, msg) {
  GPIO.write(pin, 0);
  Net.send(conn, redir);
  Net.disconnect(conn);
}, true);

// Send temperature readings to the cloud
Timer.set(freq, 1, function() {
  let topic = 'mOS/topic1';
  let message = JSON.stringify({
    total_ram: Sys.total_ram(),
    free_ram: Sys.free_ram()
  });
  MQTT.pub(topic, message, message.length);
}, null);

print('HTTP endpoints initialised');
