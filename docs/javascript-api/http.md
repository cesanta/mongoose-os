---
title: HTTP
---

Mongoose IoT implements a subset of node.js HTTP API. This snippet demonstrates
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


- `Http.createServer(function(req, res) {}) -> server_obj`: Create HTTP server
  instance.
- `server_obj.listen(address) -> server_obj`: Start listening on given address.
  Address could be a port number, or a `IP_ADDRESS:PORT_NUMBER` string.
- `response_obj.writeHead(code, headers) -> null`: Write response status line
  and HTTP headers
- `response_obj.write(data) -> null`: Write HTTP body data. Could be called
  multiple times.
- `response_obj.end(optional_data) -> null`: Finish sending HTTP body.
- `response_obj.serve(optional_conf_obj) -> null`: Serve static files.
This is a non-standard method that utilizes underlying Mongoose engine.
The `optional_conf_obj` controls Mongoose behavior for static file serving,
for example `{ ip_acl: "-0.0.0.0/0,+192.168/16" }`. Full list of options
is referenced in Mongoose documentation at
https://docs.cesanta.com/mongoose/latest/#/c-api/http.h/struct_mg_serve_http_opts/


- `Http.request(options_obj, callback) -> client_obj`: Create HTTP client.
- `Http.request(url, callback) -> client_obj`: Create HTTP client.
- `client_obj.end(optional_post_data) -> client_obj`: Finish sending HTTP body.


### Using HTTPS

In order to use HTTPS, specify `https` as a protocol, e.g.

```
Http.request('https://google.com', function() { print(arguments); });
```

Note that Mongoose IoT uses
[ca.pem](https://github.com/cesanta/mongoose-iot/blob/master/fw/src/fs/ca.pem)
file which holds root CA certificates
for verifying server certificates. Most popular certificate providers are
already added to that file. If there are problems with making HTTPS requests,
add respective CA to the `ca.pem` file.
