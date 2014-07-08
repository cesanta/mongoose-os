// This is an example websocket server for websocket.js embeddable C/C++ engine.
// This server implements echo service: any websocket message that arrives is
// echoed back to the client.

var options = {
  listening_port: 8000,
  //onaccept: function(conn) { print(conn.nc, ' connected\n') },
  //onclose: function(conn) { print(conn.nc, ' disconnected\n') },
  // onpoll: function(conn) { print('poll: ', conn, '\n') },
  // enable_ssl_with_certificate: 'cert.pem',
  // debug_hexdump_file: '/dev/stdout',
};

//print('TCP echo server, port ', options.listening_port, '\n');
var srv = TcpServer(options);

srv.onstart = function() {
  print('Starting on port ', srv.options.listening_port, '\n');
  //print(this, '\n');
};

srv.onmessage = function(conn) {
  conn.send(conn.data);   // Echo incoming message back to the client
  if (conn.data.indexOf('quit') >= 0) exit(0);  // terminate program
  if (conn.data == '\n') return false;          // close connection
  conn.data = '';  // Discard all data from the incoming buffer
};

srv.run();
