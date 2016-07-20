---
title: TCP
---

Mongoose IoT Platform implements a subset of Node.js Net API (but uses `tcp` name, instead of `net`). This snippet demonstrates
what is supported for client and server:

```javascript
var server = tcp.createServer({}, function(sock) {
  print("New incoming conenction");
  sock.on("data", function(data) {
    print("New data received", data);
    sock.write(data, null, function() {
      print("Data sent back");
    });
    sock.end();
  });
});

server.listen(19000, "127.0.0.1", 0, function() {
  print("Started to listen");
});

var client = tcp.connect(19000, "127.0.0.1", function() {
  print("Client connected");
  client.write("Hello");
});
client.on("close", function() {
  print("Client socket is closed");
});
client.on("data", function(data) {
  print("Client received new data", data);
  client.end();
});
```

- `tcp.connect(options, optional_callback) -> socket_obj`,  `tcp.connect(port, host, optional_callback) ->socket_obj`,<br>
  `tcp.createConnection(options, optional_callback) -> socket_obj`, `tcp.createConnection(port, host, optional_callback) -> socket_obj`: Returns a new tcp.Socket and automatically connects to the supplied port and host
- `tcp.createServer(optinal_options, optional_callback) -> server_obj`: Creates a new server
- `server_obj.close(optional_callback])`: Stops the server from accepting new connections and keeps existing connections
- `server_obj.getConnections(callback)` : Asynchronously gets the number of concurrent connections on the server
- `server.listen(options, optinal_callback])`, `server.listen(port, hostname, optinal_backlog, optinal_callback)`: Begins accepting connections on the specified port and hostname.
- `server.on(event, callback)`: Registers an event hanlder. The following events are supported:<br>
  * `error`:  emitted whenever any error occurs<br>
  * `close`: emitted after a server is closed<br>
  * `connection`: emitted when a new connection is made<br>
  * `listening`: emitted when the server has been bound after calling `server_obj.listen`
- `new tcp.Socket(options) -> socket_obj`: Constructs a new socket object
- `socket_obj.connect(options, optional_callback])`, `socket_obj.connect(port, host, optional_callback)`: Opens the connection for a given socket
- `socket_obj.write(data, optional_encoding, optional_callback])`: Sends data on the socket
- `socket_obj.end(optional_data, optional_encoding])`: Closes the socket. If data is specified, it is the equivalent to calling socket.write(data, encoding) followed by socket.end().
- `socket_obj.destroy()`: Closes the socket
- `socket_obj.pause()`: Pauses the reading of data
- `socket_obj.resume()`: Resumes reading after a call to pause().
- `socket_obj.setTimeout(timeout, optional_callback])`: Sets the socket to timeout after timeout milliseconds of inactivity on the socket. 1000ms is the minimum supported and any number given will be rounded to the nearest multiple of 1000.
- `socket_obj.on(event, callback)`: Registers an event hanlder. The following events are supported:<br>
  * `error`:  emitted whenever any error occurs<br>
  * `close`: emitted after a socket is closed<br>
  * `connect`: emitted when a socket connection is successfully established<br>
  * `data`: emitted when data is received<br>
  * `drain`: emitted when the write buffer becomes empty<br>
  * `timeout`: emitted if the socket times out from inactivity<br>
