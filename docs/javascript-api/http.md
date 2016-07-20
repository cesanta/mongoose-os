---
title: HTTP
---

Mongoose IoT Platform implements a subset of Node.js HTTP API. This snippet demonstrates
what is supported for client and server:

```javascript
var server = Http.createServer(function(req, res) {
  if (req.url == '/hello') {
    res.writeHead(200, {'Content-Type': 'text/plain'});
    res.write(req);
    res.end('\n');
  } else {
    res.serve({ document_root: '/tmp' });
  }
}).listen('127.0.0.1:8000');

var client = Http.request({
  hostname: '127.0.0.1',
  port: 8000,
  path: '/hello',
  method: 'POST',
  headers: { MyHeader: 'hi there' }
}, function(res) {
  print('in reply handler', res);
}).end('this is POST data');
```


- `Http.createServer(function(req, res) {}) -> server_obj`: Creates a HTTP server
  instance.
- `server_obj.listen(address) -> server_obj`: Starts listening on a given address.
  The address could be a port number or an `IP_ADDRESS:PORT_NUMBER` string.
- `server_obj.on(event, function(id) {})` -> Registers an event hanlder. The following events are supported:<br>
  * `close`: emitted after a connection is closed<br>
  * `connection`: emitted when a new connection is made<br>
  * `error`:  emitted whenever an error occurs<br>
  NOTE: The callback function has a non-standard signature. It has an `id` parameter, which is a number, but not an object.
- `server_obj.destroy`: Disconnects all clients and closes the server. Non-standard method.
- `response_obj.writeHead(code, headers) -> null`: Writes a response status line
  and HTTP headers
- `response_obj.write(data) -> null`: Writes HTTP body data. Could be called
  multiple times.
- `response_obj.end(optional_data) -> null`: Finishes sending HTTP body.
- `response_obj.serve(optional_conf_obj) -> null`: Serves static files.
This is a non-standard method that utilises the underlying Mongoose engine.
The `optional_conf_obj` controls Mongoose's behaviour for static file serving,
for example `{ ip_acl: "-0.0.0.0/0,+192.168/16" }`. The full list of options
is referenced in Mongoose documentation at
https://docs.cesanta.com/mongoose/latest/#/c-api/http.h/struct_mg_serve_http_opts/


- `Http.request(options_obj, callback) -> client_obj`: Creates HTTP client.
- `Http.request(url, callback) -> client_obj`: Creates HTTP client.
- `client_obj.end(optional_post_data) -> client_obj`: Finishes sending HTTP body.


### Using HTTPS

In order to use HTTPS, specify `https` as a protocol, e.g.

```
Http.request('https://google.com', function() { print(arguments); });
```

Note that Mongoose IoT Platform uses a
[ca.pem](https://github.com/cesanta/mongoose-iot/blob/master/fw/src/fs/ca.pem)
file which holds root CA certificates
for verifying server certificates. The most popular certificate providers are
already added to that file. If there are problems with making HTTPS requests,
add a respective CA to the `ca.pem` file.
