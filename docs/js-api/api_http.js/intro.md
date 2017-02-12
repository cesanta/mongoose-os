---
title: "HTTP"
items:
---

 Note: if you want to create RESTful handlers, use RPC API instead.
 With RPC, you'll get a RESTful endpoint for free, plus many other extras
 like ability to call your API via other transports like Websocket, MQTT,
 serial, etc.



 **`HTTP.get_system_server()`** - return an opaque pointer variable,
 a handler of the built-in HTTP server.



 **`HTTP.bind(portStr)`** - start HTTP listener. Return an opaque pointer.
 Avoid using this, use `HTTP.get_system_server()` instead.



 **`HTTP.add_endpoint(listener, uri, handler)`** - register URI
 handler. Avoid using this, use `RPC.addHandler()` instead.
 Handler function is Mongoose event handler, which receives an opaque
 connection, event number, and event data pointer.
 Events are `HTTP.EV_REQUEST`, `HTTP.EV_RESPONSE`.
 Example:
 ```javascript
 let server = HTTP.get_system_server();
 HTTP.add_endpoint(server, '/my/api', function(conn, ev, ev_data) {
   if (ev === HTTP.EV_REQUEST) {
     Net.send(conn, 'HTTP/1.0 200 OK\n\n  hello! \n');
     Net.disconnect(conn);
   }
 }, null);
 ```

