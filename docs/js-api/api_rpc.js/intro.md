---
title: "RPC"
items:
---

 RPC means Remote Procedure Call. In Mongoose OS, that is simply a C
 (or JS) function that:

 - Has a name, for example "GPIO.Toggle",
 - Takes a JSON object with function arguments,
 - Gives back a JSON object with results.


 These JSON messages could be carried out by many different channels.
 Mongoose OS by default supports Serial (UART), HTTP, WebSocket, MQTT channels.



 **`RPC.addHandler(name, handler)`** -
 add RPC handler. `name` is a string like `'MyMethod'`, `handler`
 is a callback function which takes `args` arguments object.

 Return value: none.

 Example:
 ```javascript
 RPC.addHandler('Sum', function(args) {
   return args.a + args.b;
 });
 ```
 The installed handler is available over Serial, Restful, MQTT, Websocket,
 for example over Websocket:
 ```bash
 $ mos --port ws://192.168.0.206/rpc call Sum '{"a":1, "b": 2}'
 3
 ```
 Or, over familiar RESTful call:
 ```bash
 $ curl -d '{"a":1, "b": 2}' 192.168.0.206/rpc/Sum
 3
 ```



 **`RPC.Call(dst, method, args, callback)`** - call remote RPC service.

 Return value: 1 in case of success, 0 otherwise.

 If `dst` is empty, connected server is implied. `method` is a string
 like "MyMethod", `callback` is a callback function which takes the following
 arguments: res (results object), err_code (0 means success, or error code
 otherwise), err_msg (error messasge for non-0 error code), userdata.

