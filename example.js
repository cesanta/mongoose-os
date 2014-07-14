// This is an example TCP server for websocket.js embeddable C/C++ engine.
// This server implements TCP echo service: any message that arrives is
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
  //conn.send(conn.data);   // Echo incoming message back to the client
  //conn.data = '';         // Discard all data from the incoming buffer
  var len = conn.data.indexOf('\r\n\r\n');
  print(conn.data, 'len :', len, '\n');
};

//srv.onaccept = function(conn) { print(conn.nc, ' connected\n') };
//srv.onclose = function(conn) { print(conn.nc, ' disconnected\n') };
//srv.onpoll = function(conn) { print('poll: ', conn, '\n') };
//srv.onpoll = function(conn) { print('poll, conns: ', srv.connections, '\n') };

srv.run();
