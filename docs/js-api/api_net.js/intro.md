---
title: "Net"
items:
---

Raw TCP/UDP API.



## **`Net.bind(addressStr, handler)`**
Bind to an address. Return value:
an opaque connection pointer which should be given as a first argument to
some `Net` functions. A handler function is a Mongoose event
handler, that receives connection, event, and event data. Events are:
`Net.EV_POLL`, `Net.EV_ACCEPT`, `Net.EV_CONNECT`, `Net.EV_RECV`,
`Net.EV_SEND`, `Net.EV_CLOSE`, `Net.EV_TIMER`. Example:
```javascript
Net.bind(':1234', function(conn, ev, ev_data) {
  print(ev);
}, null);
```



## **`Net.connect(addr, handler, userdata)`**
Connect to a remote host.
Return value: an opaque connection pointer which should be given as a first argument to
some `Net` functions.

The addr format is `[PROTO://]HOST:PORT`. `PROTO` could be `tcp` or
`udp`. `HOST` could be an IP address or a host name. If `HOST` is a name,
it will be resolved asynchronously.

Examples of valid addresses: `google.com:80`, `udp://1.2.3.4:53`,
`10.0.0.1:443`, `[::1]:80`.

Handler is a callback function which takes the following arguments:
`(conn, event, event_data, userdata)`.
conn is an opaque pointer which should be used as a first argument to
`Net.send()`, `Net._send()`, `Net.close()`.
event is one of the following:
- `Net.EV_POLL`
- `Net.EV_ACCEPT`
- `Net.EV_CONNECT`
- `Net.EV_RECV`
- `Net.EV_SEND`
- `Net.EV_CLOSE`
- `Net.EV_TIMER`
event_data is additional data; handling of it is currently not possible
from mJS.
userdata is the value given as a third argument to `Net.connect()`.



## **`Net.connect_ssl(addr, handler, userdata, cert, ca_cert)`**
The same as `Net.connect`, but establishes SSL connection
Additional parameters are:
- `cert` is a client certificate file name or "" if not required
- `key` is a client key file name or "" if not required
- `ca_cert` is a CA certificate or "" if peer verification is not required.
The certificate files must be in PEM format.



## **`Net.close(conn)`**
Send all pending data to the remote peer,
and disconnect when all data is sent.
Return value: none.



## **`Net.send(conn, data)`**
Send data to the remote peer. `data` is an mJS string.
Return value: none.

