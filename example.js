// This is an example HTTP/Websocket server
// $Date$

function parseHttpRequest(str) {
  var ind = str.indexOf('\r\n\r\n');              // Where HTTP headers end
  if (ind < 0 && str.length > 8192) return null;  // Request too big
  if (ind < 14 && ind > 0) return null;           // Request too small
  if (ind > 0) {
    var request = str.substr(0, ind + 1);
    var lines = request.split(/\s*\n\s*/);
    var firstLine = lines[0].split(' ');
    var parsed = { method: firstLine[0], uri: firstLine[1], headers: {} };
    for (var i = 1; i < lines.length; i++) {
      var pair = lines[i].split(': ', 2);
      parsed.headers[pair[0]] = pair[1];
    }
    return parsed;
  }
  return true;
};

var httpServerConfig = {
  document_root: '.',    // Current directory
  listening_port: 8000
};

var handleWebsocketData = function(conn) {
};

var handleHttpRequest = function(conn) {
  var data, request, path, f;

  print(conn.data);
  if (conn.is_websocket) return handleWebsocketData(conn);

  request = parseHttpRequest(conn.data);
  if (!request) return true;   // Bad request, close the connection

  if (request.uri) {
    var uri = request.uri;
    if (request.uri.match(/\/$/)) request.uri += 'index.html';
    path = httpServerConfig.document_root + request.uri;
    print('Serving [', uri, ']');
    f = open(path, 'r');
    if (f) {
      conn.send('HTTP/1.0 200 OK\r\nConnection: close\r\n\r\n');
      while (data = f.read()) conn.send(data);
      f.close();
    } else {
      conn.send('HTTP/1.0 404 Not Found\r\n\r\n');
    }
    //var numConns = conn.server.connections.keys().length;
    //conn.send('HTTP/1.0 200 OK\r\n\r\n',
    //          'URI: ', req.uri, '\nNum connections: ', req, '\n');
    return true;   // Close connection
  }
  // Return nothing (keep connection open) and buffer more conn.data
};

// Configure and start
var srv = NetEventManager({
  listening_port: httpServerConfig.listening_port,
  // enable_ssl_with_certificate: 'cert.pem',
  // debug_hexdump_file: '/dev/stdout',
  onstart: function() { print('Starting on port ', this.listening_port); },
  onaccept: function(conn) { print(conn.nc, ' connected'); },
  onmessage: handleHttpRequest,
  // onpoll: function(conn) { print('poll, conns: ', srv.connections, '\n'),
  onclose: function(conn) { print(conn.nc, ' disconnected'); }
});
srv.run();
