// This is an example websocket server for websocket.js embeddable C/C++ engine.
// This server implements echo service: any websocket message that arrives is
// echoed back to the client.

var tcp_echo_server_options = {
  listening_port: 8000,
  onmessage: function(conn) {
    conn.send(conn.data);
    conn.discard(conn.num_bytes);
  },
  onaccept: function(conn) { print('accept: ', conn, '\n') },
  onpoll: function(conn) { print('poll: ', conn, '\n') },
  onclose: function(conn) { print('close: ', conn, '\n') },
  // enable_ssl_with_certificate: 'cert.pem',
  // debug_hexdump_file: '/dev/stdout',
};

print('TCP echo server, port ', tcp_echo_server_options.listening_port, '\n');
RunTcpServer(tcp_echo_server_options);
//RunTcpServer({ listening_port: 8000 });

//print('Starting Websocket echo server on port ', options.listening_port, '\n');
//RunWebsocketServer(options);
