// This is an example websocket server for websocket.js
// It implements echo service: any websocket message that arrives is
// echoed back to the client.

var options = {
  hexdump_file: '/dev/stdout'
};

var ws = Websocket(8000, options);

print('Starting, namespace: ', this, '\n');

ws.run();
