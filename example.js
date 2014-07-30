// This is an example HTTP/Websocket server

load('http.js');

//print(JSON.stringify(parseHttpRequest('GET / HTTP/1.0\r\n\r\n')));
// NetEventManager = { run: function() {} }

var srv = NetEventManager({
  listening_port: 8000,
  // enable_ssl_with_certificate: 'cert.pem',
  // debug_hexdump_file: '/dev/stdout',
  onstart: function() {
    print('Starting on port ', this.listening_port);
  },
  onaccept: function(conn) {
    print(conn.nc, ' connected');
  },
  onmessage: function(conn) {
    var req = parseHttpRequest(conn.data);
    if (!req) return false;   // Bad request, close the connection
    if (req.uri) {  
      conn.send('HTTP/1.0 200 OK\r\n\r\n',
                'URI: ', req.uri, '\nReceived request:\n\n', req.request);
      return false;   // Close connection
    }
    // Return nothing (keep connection open) and buffer more conn.data
  },
  // onpoll: function(conn) { print('poll, conns: ', srv.connections, '\n'),
  onclose: function(conn) {
    print(conn.nc, ' disconnected');
  }
});

srv.run();