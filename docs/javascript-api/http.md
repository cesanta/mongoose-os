---
title: HTTP
---

Smart.js implements a subset of node.js HTTP API. This snippet demonstrates
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
  instance
- `server_obj.listen(address) -> server_obj`: Start listening on given address.
  Address could be a port number, or a `IP_ADDRESS:PORT_NUMBER` string.
- `response_obj.writeHead(code, headers) -> null`: Write response status line
  and HTTP headers
- `response_obj.write(data) -> null`: Write HTTP body data. Could be called
  multiple times.
- `response_obj.end(optional_data) -> null`: Finish sending HTTP body


- `Http.request(options_obj, callback) -> client_obj`: Create HTTP client
- `client_obj.end(optional_post_data) -> client_obj`: Finish sending HTTP body

