// This example demonstrates how to create a TCP echo server.
//
// To try this example,
//   1. Download `mos` tool from https://mongoose-os.com/software.html
//   2. Run `mos` tool and install Mongoose OS
//   3. In the UI, navigate to the `Examples` tab and load this example

// Load Mongoose OS API
load('api_net.js');
load('api_http.js');

// let listener = HTTP.bind('8000');
let listener = HTTP.get_system_server();
HTTP.add_endpoint(listener, '/foo', function(conn, ev, msg) {
  Net.send(conn, 'HTTP/1.0 200 OK\r\n\r\n');
  Net.send(conn, HTTP.param(msg, HTTP.MESSAGE));
  Net.disconnect(conn);
}, true);
