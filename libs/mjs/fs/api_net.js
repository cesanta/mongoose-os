load('api_events.js');

let Net = {
  _rb: ffi('void *mgos_get_recv_mbuf(void *)'),
  _mptr: ffi('void *mgos_get_mbuf_ptr(void *)'),
  _glen: ffi('int mgos_get_mbuf_len(void *)'),
  _mrem: ffi('void mbuf_remove(void *, int)'),
  _isin: ffi('bool mgos_is_inbound(void *)'),

  _bind: ffi('void *mgos_bind(char *, void (*)(void *, int, void *, userdata), userdata)'),
  _c: ffi('void *mgos_connect(char *, void (*)(void *, int, void *, userdata), userdata)'),
  _cs: ffi('void *mgos_connect_ssl(char *, void (*)(void *, int, void *, userdata), userdata, char *, char *, char *)'),
  _send: ffi('void mg_send(void *, void *, int)'),
  _ctos: ffi('int mg_conn_addr_to_str(void *, char *, int, int)'),

  // Return string contained in connection's recv_mbuf
  _rbuf: function(conn) {
    let rb = this._rb(conn);
    return mkstr(this._mptr(rb), this._glen(rb));
  },

  // **`Net.ctos(conn, local, ip, port)`**
  // Convert address of a connection `conn` to string. Set `local` to
  // `true` to stringify local address, otherwise `false` to stringify remote.
  // Set `ip` to `true` to stringify IP, `port` to stringify port. Example:
  // ```javascript
  // print('Connection from:', Net.ctos(conn, false, true, true));
  // ```
  ctos: function(conn, local, ip, port) {
    let buf = '                              ';
    let flags = (local ? 0 : 4) | (ip ? 1 : 0) | (port ? 2 : 0);
    let n = this._ctos(conn, buf, buf.length, flags);
    return buf.slice(0, n);
  },

  // **`Net.discard(conn, len)`**
  // Remove initial `len` bytes of data from the connection's `conn`
  // receive buffer in order to discard that data and reclaim RAM to the system.
  discard: function(conn, len) {
    this._mrem(this._rb(conn), len);
  },

  // Event handler. Expects an object with connect/data/close/event user funcs.
  _evh: function(conn, ev, edata, obj) {
    if (ev === 0) return;

    if (ev === 1 || ev === 2) {
      if (obj.onconnect) obj.onconnect(conn, edata, obj);
    } else if (ev === 3) {
      if (obj.ondata) obj.ondata(conn, Net._rbuf(conn), obj);
    } else if (ev === 5) {
      if (obj.onclose) obj.onclose(conn, obj);
      let inb = Net._isin(conn);  // Is this an inbound connection ?
      if (!inb) ffi_cb_free(Net._evh, obj);
    } else if (ev >= 6) {
      if (obj.onevent) obj.onevent(conn, Net._rbuf(conn), ev, edata, obj);
    }
  },

  // ## **`Net.serve(options)`**
  // Start TCP or UDP server. `options` is an object:
  // ```javascript
  // {
  //    // Required. Port to listen on, 'tcp://PORT' or `udp://PORT`.
  //    addr: 'tcp://1234',
  //    // Optional. Called when connection is established.
  //    onconnect: function(conn) {}, 
  //    // Optional. Called when new data is arrived.
  //    ondata: function(conn, data) {},
  //    // Optional. Called when protocol-specific event is triggered.
  //    onevent: function(conn, data, ev, edata) {},
  //    // Optional. Called when the connection is about to close.
  //    onclose: function(conn) {},
  //    // Optional. Called when on connection error.
  //    onerror: function(conn) {},
  // }
  // ```
  // Example - a UDP echo server. Change `udp://` to `tcp://` to turn this
  // example into the TCP echo server:
  // ```javascript
  // Net.serve({
  //   addr: 'udp://1234',
  //   ondata: function(conn, data) {
  //     print('Received from:', Net.ctos(conn, false, true, true), ':', data);
  //     Net.send(conn, data);            // Echo received data back
  //     Net.discard(conn, data.length);  // Discard received data
  //   },
  // });
  // ```
  serve: function(obj) {
    return this._bind(obj.addr, this._evh, obj);
  },

  // ## **`Net.connect(options)`**
  // Connect to a remote host. `options` is the same as for the `Net.serve`.
  // The addr format is `[PROTO://]HOST:PORT`. `PROTO` could be `tcp` or
  // `udp`. `HOST` could be an IP address or a host name. If `HOST` is a name,
  // it will be resolved asynchronously.
  //
  // Examples of valid addresses: `google.com:80`, `udp://1.2.3.4:53`,
  // `10.0.0.1:443`, `[::1]:80`.
  connect: function(obj) {
    if (obj.ssl) {
      return this._cs(obj.addr, this._evh, obj, obj.cert || '', obj.key || '', obj.ca_cert || 'ca.pem');
    } else {
      return this._c(obj.addr, this._evh, obj);
    }
  },

  // ## **`Net.close(conn)`**
  // Send all pending data to the remote peer,
  // and disconnect when all data is sent.
  // Return value: none.
  close: ffi('void mgos_disconnect(void *)'),

  // ## **`Net.send(conn, data)`**
  // Send data to the remote peer. `data` is an mJS string.
  // Return value: none.
  send: function(c, msg) {
    return Net._send(c, msg, msg.length);
  },

  // ## **`Net.EVENT_GRP`**
  // Net events group, to be used with `Event.addGroupHandler()`. Possible
  // events are:
  // - `Net.STATUS_DISCONNECTED`
  // - `Net.STATUS_CONNECTING`
  // - `Net.STATUS_CONNECTED`
  // - `Net.STATUS_GOT_IP`
  EVENT_GRP: Event.baseNumber("NET"),
};

Net.STATUS_DISCONNECTED = Net.EVENT_GRP + 0;
Net.STATUS_CONNECTING   = Net.EVENT_GRP + 1;
Net.STATUS_CONNECTED    = Net.EVENT_GRP + 2;
Net.STATUS_GOT_IP       = Net.EVENT_GRP + 3;
