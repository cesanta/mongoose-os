---
title: "URL"
items:
---

Note: if you want to create RESTful handlers, use RPC API instead.
With RPC, you'll get a RESTful endpoint for free, plus many other extras
like ability to call your API via other transports like Websocket, MQTT,
serial, etc.

load('api_net.js');

let URL = {
## **`URL.parse(url)`**
Parse URL string, return and object with `ssl`, `addr`, `uri` keys.
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



## **`URL.parse(url)`**
Parse URL string, return and object with `ssl`, `addr`, `uri` keys.



Get `struct http_message *` foreign ptr and offset, return JS string.



## **`HTTP.query(options)`**
Send HTTP request. Options object accepts the following fields:
`url` - mandatory URL to fetch, `success` - optional callback function 
that receives reply body, `error` - optional error callback that receives
error string, `data` - optional object with request parameters.
By default, `GET` method is used, unless `data` is specified.
Example:
```javascript
HTTP.query({
  url: 'http://httpbin.org/post',
  headers: { 'X-Foo': 'bar' },  // Optional - headers
  data: {foo: 1, bar: 'baz'},   // Optional. If set, POST is used
  success: function(body, full_http_msg) { print(body); },
  error: function(err) { print(err); },  // Optional
});
```

