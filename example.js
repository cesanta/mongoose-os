// This is an example websocket server for websocket.js embeddable C/C++ engine.
// This server implements echo service: any websocket message that arrives is
// echoed back to the client.

var options = {
  listening_port: 8000,
  onmessage: function(conn) {
    if (conn.data == 'qqq\n') { std.exit(2); }
    conn.send(conn.data);
    conn.discard(conn.data.length);
  },
  onaccept: function(conn) { std.print(conn.nc, ' connected\n') },
  onclose: function(conn) { std.print(conn.nc, ' disconnected\n') },
  // onpoll: function(conn) { print('poll: ', conn, '\n') },
  // enable_ssl_with_certificate: 'cert.pem',
  // debug_hexdump_file: '/dev/stdout',
};

std.print('TCP echo server, port ', options.listening_port, '\n');
RunTcpServer(options);
//RunTcpServer({ listening_port: 8000 });

//print('Starting Websocket echo server on port ', options.listening_port, '\n');
//RunWebsocketServer(options);
