// This example demonstrates how to create a TCP echo server.

// Load Mongoose OS API
load('api_net.js');

Net.bind('1234', function(conn, ev, ev_data) {
  if (ev != Net.EV_ACCEPT) return;
  let msg = JSON.stringify({a: 1, b: 'опля'});
  Net.send(conn, msg, msg.length);
  Net.disconnect(conn);
}, true);
