// This is an example websocket server for websocket.js embeddable C/C++ engine.
// This server implements echo service: any websocket message that arrives is
// echoed back to the client.

var options = {
  listening_port: '8000',
  onaccept: function(conn) {
    print('Accepted new connection: ', conn, '\n');
  },
  onmessage: function(conn) {
    print('Received message: ', conn, '\n');
    conn.send(conn.data);
    conn.discard(conn.num_bytes);
  },
  onclose: function(conn) {
    print('Received message: ', conn, '\n');
  },
  // enable_ssl_with_certificate: 'cert.pem',
  // debug_hexdump_file: '/dev/stdout',
};

print('Starting websocket echo server on port ', options.listening_port, '\n');
RunTcpServer(options);

//var websocket_server = WebsocketServer(options);
//websocket_server.run();
