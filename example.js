// This is an example HTTP/Websocket server

var options = {
  listening_port: 8000,
  // enable_ssl_with_certificate: 'cert.pem',
  // debug_hexdump_file: '/dev/stdout',
};

var srv = NetEventManager(options);

srv.onstart = function() {
  print('Starting on port ', srv.options.listening_port, '\n');
};

srv.onmessage = function(conn) {
  var ind = conn.data.indexOf('\r\n\r\n');  // Find the end of HTTP request
  if (ind < 0 && conn.data.length > 8192) return false;   // Request too big
  if (ind < 15 && ind > 0) return false;                  // Request too small

  // If request is fully buffered, parse it, and send the reply
  if (ind > 0) {
    var request = conn.data.substr(0, ind + 1);
    var lines = request.split('\r\n');
    var ar = lines[0].split(' ');
    conn.send('HTTP/1.0 200 OK\r\n\r\n',
              'URI: ', ar[1], '\nReceived request:\n\n', request);
    return false;   // close the connection
  }

  // Otherwise, return nothing (keep connection open) and buffer more data
};

//srv.onaccept = function(conn) { print(conn.nc, ' connected\n') };
//srv.onclose = function(conn) { print(conn.nc, ' disconnected\n') };
//srv.onpoll = function(conn) { print('poll, conns: ', srv.connections, '\n') };

srv.run();