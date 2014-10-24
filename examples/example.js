// $Date$

/*
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

em.listen(80, new HttpServer({ root: '.', }));
em.connect()
var httpServer = new HttpServer(em, { listen: 8000, root: '.' });
*/

var http = load('modules/http.js');


var em = new EventManager();
//var web_server = new http.HttpServer({ root: '.' });
var web_server = new http.HttpServer();
/*
print(em);

em.bind(8000, web_server);
em.run();
*/