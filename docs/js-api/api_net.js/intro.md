---
title: "Net"
items:
---

 Raw TCP/UDP API.



 **`Net.bind(addressStr, handler)`** - bind to an address. Return value:
 an opaque pointer. A handler function is a Mongoose event handler,
 that receives connection, event, and event data. Events are:
 `Net.EV_POLL`, `Net.EV_ACCEPT`, `Net.EV_CONNECT`, `Net.EV_RECV`,
 `Net.EV_SEND`, `Net.EV_CLOSE`, `Net.EV_TIMER`. Example:
 ```javascript
 Net.bind(':1234', function(conn, ev, ev_data) {
   print(ev);
 }, null);
 ```



 **`Net.disconnect(conn)`** - send all pending data to the remote peer,
 and disconnect when all data is sent.



 **`Net.send(conn, data)`** - send data to the remote peer.

