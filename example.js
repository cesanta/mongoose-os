// This is an example websocket server for websocket.js embeddable C/C++ engine.
// This server implements echo service: any websocket message that arrives is
// echoed back to the client.

var options = {
  listening_port: 8000,
  // enable_ssl_with_certificate: 'cert.pem',
  // debug_hexdump_file: '/dev/stdout',
};

var srv = TcpServer(options);

srv.onstart = function() {
  print('Starting on port ', srv.options.listening_port, '\n');
};

srv.onmessage = function(conn) {
  conn.send(conn.data);   // Echo incoming message back to the client
  if (conn.data.indexOf('quit') >= 0) exit(0);  // terminate the whole program
  if (conn.data == '\n') return false;          // close this connection
  conn.data = '';  // Discard all data from the incoming buffer
};

//srv.onaccept = function(conn) { print(conn.nc, ' connected\n') };
//srv.onclose = function(conn) { print(conn.nc, ' disconnected\n') };
//srv.onpoll = function(conn) { print('poll: ', conn, '\n') };
//srv.onpoll = function(conn) { print('poll, conns: ', srv.connections, '\n') };

srv.run();
