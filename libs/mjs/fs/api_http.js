load('api_net.js');

let URL = {
  // ## **`URL.parse(url)`**
  // Parse URL string, return and object with `ssl`, `addr`, `uri` keys.
  //
  // Example:
  // ```javascript
  // print(JSON.stringify(URL.parse('https://a.b:1234/foo?bar')));
  // // Prints: {"uri":"/foo?bar","addr":"a.b:1234","ssl":true}
  // ```
  parse: function(url) {
    let ssl = false, addr, port = '80', uri = '/', app = true;
    if (url.slice(0, 8) === 'https://') {
      port = '443';
      ssl = true;
      url = url.slice(8);
    }
    if (url.slice(0, 7) === 'http://') {
      url = url.slice(7);
    }
    addr = url;
    for (let i = 0; i < url.length; i++) {
      let ch = url[i];
      if (ch === ':') app = false;
      if (ch === '/') {
        addr = url.slice(0, i);
        uri = url.slice(i);
        break;
      }
    }
    if (app) addr += ':' + port;
    return {ssl: ssl, addr: addr, uri: uri};
  },
};

let HTTP = {
  _getm: ffi('void *mgos_get_msg_ptr(void *)'),
  _getb: ffi('void *mgos_get_body_ptr(void *)'),
  _mgp: ffi('void *mgos_get_mgstr_ptr(void *)'),
  _mgl: ffi('int mgos_get_mgstr_len(void *)'),

  _c: ffi('void *mgos_connect_http(char *, void (*)(void *, int, void *, userdata), userdata)'),
  _cs: ffi('void *mgos_connect_http_ssl(char *, void (*)(void *, int, void *, userdata), userdata, char *, char *, char *)'),
  _sp: ffi('void mg_set_protocol_http_websocket(void *)'),

  _mstr: function(hmptr, func) {
    let mgstr = func(hmptr);
    return mkstr(this._mgp(mgstr), this._mgl(mgstr));
  },

  // ## **`HTTP.query(options);`**
  // Send HTTP request. Options object accepts the following fields:
  // `url` - mandatory URL to fetch, `success` - optional callback function 
  // that receives reply body, `error` - optional error callback that receives
  // error string, `data` - optional object with request parameters.
  // By default, `GET` method is used. If `data` is specified, POST method
  // is used, the `data` object gets `JSON.stringify()`-ed and used as a
  // HTTP message body.
  //
  // In order to send HTTPS request, use `https://...` URL. Note that in that
  // case `ca.pem` file must contain CA certificate of the requested server.
  //
  // Example:
  // ```javascript
  // HTTP.query({
  //   url: 'http://httpbin.org/post',
  //   headers: { 'X-Foo': 'bar' },     // Optional - headers
  //   data: {foo: 1, bar: 'baz'},      // Optional. If set, JSON-encoded and POST-ed
  //   success: function(body, full_http_msg) { print(body); },
  //   error: function(err) { print(err); },  // Optional
  // });
  // ```
  query: function(opts) {
    let url = URL.parse(opts.url || '');
    return Net.connect({
      addr: url.addr,
      ssl: url.ssl,
      u: url,
      opts: opts,
      onconnect: function(conn, edata, ud) {
        let opts = ud.opts;
        let body = opts.data || '';
        if (typeof(body) !== 'string') body = JSON.stringify(body);
        let meth = body ? 'POST' : 'GET';
        let host = 'Host: ' + ud.u.addr + '\r\n';
        let cl = 'Content-Length: ' + JSON.stringify(body.length) + '\r\n';
        let hdrs = opts.headers || {};
        for (let k in hdrs) {
          cl += k + ': ' + hdrs[k] + '\r\n';
        }
        let req = meth + ' ' + ud.u.uri + ' HTTP/1.0\r\n' + host + cl + '\r\n';
        Net.send(conn, req);
        Net.send(conn, body);
        HTTP._sp(conn);
      },
      onevent: function(conn, buf, ev, edata, ud) {
        if (ev === 101 && ud.opts.success) {
          let body = HTTP._mstr(edata, HTTP._getb);
          let full = HTTP._mstr(edata, HTTP._getm);
          ud.opts.success(body, full);
          ud.ok = true;
        }
      },
      onclose: function(conn, ud) {
        let opts = ud.opts;
        if (!ud.ok && opts.error) opts.error('', 'Request failed', opts);
      },
    });
  },
};
