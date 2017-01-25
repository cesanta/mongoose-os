// This example demonstrates how to create a TCP echo server.
//
// To try this example,
//   1. Download `mos` tool from https://mongoose-iot.com/software.html
//   2. Run `mos` tool and install Mongoose OS
//   3. In the UI, navigate to the `Examples` tab and load this example

// Load Mongoose OS API
load('api_net.js');
load('api_http.js');

let port = '8000';
let listener = HTTP.bind(port);
// let listener = HTTP.get_system_server();
HTTP.serve(listener, '/foo', function(conn, msg) {
  Net.send(conn, 'HTTP/1.0 200 OK\r\n\r\n');
  Net.send(conn, HTTP.param(msg, HTTP.MESSAGE));  // Echo the request back
  Net.disconnect(conn);
});

print('HTTP server is listening on port ', port);
