// This example demonstrates how to react on a button press
// by sending an HTTP GET message.
//
// To try this example,
//   1. Download & install `mos` tool from https://mongoose-os.com/software.html
//   2. In the UI, navigate to the `Examples` tab and load this example
//
// Note: HTTPS accesses require correct CA cert loaded, see `mos config-get http`
// The default one contains https://letsencrypt.org/ certificate.

// Load Mongoose OS API
load('api_gpio.js');
load('api_http.js');
load('api_net.js');
load('api_sys.js');

let addr = 'mongoose-os.com:80';  // web server host:port

let cb = function(conn, ev, ev_data, data) {
  if (ev === Net.EV_POLL) return;  // Timer event - ignore
  if (ev === HTTP.EV_RESPONSE) {
    let response = HTTP.param(ev_data, HTTP.MESSAGE);
    print('response', response);
    Net.close(conn);
  }
  if (ev === Net.EV_CONNECT) {
    print('connected:', Sys.peek(ev_data, 0) === 0 ? 'yes' : 'no');
  }
  if (ev === Net.EV_CLOSE) {
    print('closed.');
  }
};

let pin = 0;
GPIO.set_button_handler(pin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 200, function(x) {
  print('Sending request to', addr, 'RAM: ', Sys.free_ram());
  let conn = HTTP.connect(addr, cb, null);
  Net.send(conn, 'GET /downloads/mos/version.json HTTP/1.0\r\n');
  Net.send(conn, 'Content-Length: 0\r\n');  // Must be non-0 for POST requests
  Net.send(conn, '\r\n');
}, null);
