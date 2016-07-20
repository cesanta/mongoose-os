---
title: UDP
---

Mongoose IoT Platform implements a subset of Node.js UDP API. This snippet demonstrates
what is supported for client and server:

```javascript
var serverSocket = dgram.createSocket("udp4", function(msg,rinfo) {
  print("New message", msg, "from", rinfo);
  serverSocket.send(msg, rinfo.port, rinfo.address, function() {
    print("Message sent back");
    serverSocket.close(function() {
      print("Server socket closed");
    });
  });
});
serverSocket.bind(17000, "127.0.0.1", function() {
  print("Started to listen on port 17000");
});

var clientSocket = dgram.createSocket("udp4", function(msg, rinfo) {
  print("New message", msg, "from", rinfo);
  clientSocket.close(function() {
    print("Client socket closed");
  });
});
clientSocket.send("hello", 17000, "127.0.0.1", function() {
  print("Message sent");
});
```

- `dgram.createSocket(optional_type, optional_callback) -> soocket_obj` -> socket_obj: Creates a socket instance. Note: only `type` = `udp4` is currently supported.
- `socket_obj.bind(port, optional_address[, callback])`, `socket.bind(options[, callback])`: Causes the dgram.Socket to listen for datagram messages on a named port and addres
- `socket_obj.send(msg, optional_offset, optional_length, port, address, optional_callback])`: Sends a datagram to the destination port and address.
- `socket_obj.on(event, callback)`: Registers an event hanlder. The following events are supported:<br>
  * `error`:  emitted whenever any error occurs<br>
  * `message`: emitted when a new datagram is available on a socket<br>
  * `close`: emitted after a socket is closed<br>
  * `listening`: emitted whenever a socket begins listening for datagram messages<br>



