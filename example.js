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
  // Search for the end of HTTP request
  var ind = conn.data.indexOf('\r\n\r\n');

  // If request is too big, close the connection
  if (ind < 0 && conn.data.length > 8192) return false;
  if (ind < 15 && ind > 0) return false;

  // If request is fully buffered, parse it, and send the reply
  if (ind > 0) {
    var request = conn.data.substr(0, ind + 1);
    var lines = request.split('\r\n');
    conn.send('HTTP/1.0 200 OK\r\n\r\n', 'Received request:\n\n', request);
    return false;   // close the connection
  }

  // Otherwise, return nothing (keep connection open) and buffer more data
};

//srv.onaccept = function(conn) { print(conn.nc, ' connected\n') };
//srv.onclose = function(conn) { print(conn.nc, ' disconnected\n') };
//srv.onpoll = function(conn) { print('poll, conns: ', srv.connections, '\n') };

srv.run();