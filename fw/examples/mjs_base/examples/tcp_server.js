// This example demonstrates how to create a TCP echo server.
//
// To try this example,
//   1. Download `mos` tool from https://mongoose-os.com/software.html
//   2. Run `mos` tool and install Mongoose OS
//   3. In the UI, navigate to the `Examples` tab and load this example

// Load Mongoose OS API
load('api_net.js');

let port = '1234';
Net.bind(port, function(conn, ev, ev_data) {
  if (ev != Net.EV_ACCEPT) return;
  Net.send(conn, JSON.stringify({a: 1, b: 'опля'}));
  Net.disconnect(conn);
}, true);

print('TCP server is listening on port ', port);
