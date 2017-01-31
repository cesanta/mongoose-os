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

let listener = HTTP.get_system_server();
let pin = 13;
let ok = 'HTTP/1.0 200 OK\r\n\r\n{"result":true}\n';
let freq = 10000; // Milliseconds. How often to send temperature readings

GPIO.set_mode(pin, GPIO.MODE_OUTPUT);

HTTP.add_endpoint(listener, '/status', function(conn, ev, msg) {
  Net.send(conn, 'HTTP/1.0 200 OK\r\n\r\n');
  Net.send(conn, JSON.stringify({
    on: GPIO.read(pin)
  }));
  Net.disconnect(conn);
}, true);

HTTP.add_endpoint(listener, '/on', function(conn, ev, msg) {
  GPIO.write(pin, 1);
  Net.send(conn, ok);
  Net.disconnect(conn);
}, true);

HTTP.add_endpoint(listener, '/off', function(conn, ev, msg) {
  GPIO.write(pin, 0);
  Net.send(conn, ok);
  Net.disconnect(conn);
}, true);

// Send temperature readings to the cloud
Timer.set(freq, 1, function() {
  let addr = 'mongoose.cloud:80';
  // let addr = '192.168.0.157:1234';
  print('Sending stats to', addr);
  HTTP.connect(addr, function(c, ev, ed) {
    if (ev === Net.EV_POLL) return;
    if (ev === Net.EV_CONNECT) {
      let data = JSON.stringify({ t: Sys.free_ram() });
      Net.send(c, 'POST /api/heater-cesanta/data/add HTTP/1.1\r\n');
      // TODO(lsm): add authorization header - read from a file ?
      Net.send(c, 'Content-Length: ');
      Net.send(c, JSON.stringify(data.length));
      Net.send(c, '\r\n\r\n');
      Net.send(c, data)
    } else if (ev === HTTP.EV_RESPONSE) {
      Net.disconnect(c);
      print('disconnecting...');
    }
    print('in connect, ev', c, ev);
  }, true);
}, true);

print('HTTP endpoints initialised');
