// Copyright (c) 2015 Cesanta Software Limited
// All rights reserved

var Clubby = function(arg) {
  // arg is a map:
  //  {
  //    url: URL_OF_API_BACKEND_OPTIONAL,
  //    src: CLIENT_ID,
  //    key: CLIENT_PASSWORD,
  //    log: BOOLEAN,
  //    ephemeral: BOOLEAN,          // if true do append random str to url, default true
  //    onopen: CALLBACK_OPTIONAL,   // Called when socket is connected
  //    onstart: CALLBACK_OPTIONAL,  // Called at any clubby call
  //    onstop: CALLBACK_OPTIONAL    // Called when all clubby calls are ended
  //    oncmd: CALLBACK_OPTIONAL     // Called on each incoming command
  // }

  // This is a random component part which will be added to the
  // source for all outgoing commands, to make this client unique.
  var randomString = '.' + Math.random().toString(36).substring(7);
  var defaultUrl = 'wss://api.cesanta.com:444';

  var config = this.config = $.extend(arg, {
    next_req_id: 1, // Auto-incrementing command ID
    map: {},        // request_id -> callback mapping
    hnd: {},        // command_id -> callback mapping
    rdy: [],        // callbacks called once as soon client is connected
    src: arg.src + ( arg.ephemeral && randomString || "" )
  });

  var log = function(a,b) {
    if (config.log && console && console.log) {
      console.log(a, b);
    }
  };

  var me = this;

  var reconnect = function() {
    var url = arg.url || defaultUrl;
    log('reconnecting to [' + url + ']');
    var ws = config.ws = new WebSocket(url, 'clubby.cesanta.com');

    ws.onclose = function(ev) {
      // Schedule a reconnect after 1 second
      setTimeout(reconnect, 1000);
      if (config.onclose) config.onclose();
    };

    ws.onopen = function(ev) {
      if (config.onopen) config.onopen();
      $.each(config.rdy, function(i, r) { r()});
      config.rdy = [];
    };

    // This is a non-standard callback.
    // Receives number of bytes still waiting to be sent to the client.
    ws.onsend = config.onsend;

    ws.onmessage = function(ev) {
      // Dispatch responses to the correct callback
      log('received: ', ev.data);
      var numKeys = 0, obj = JSON.parse(ev.data),  // TODO(lsm): handle parse error
          res = {}, req = { v: 1, dst: obj.src, src: config.src,
                            key: config.key, resp: [res] };

      var error = function(e) {
        res.status = 1;
        res.status_msg = e;
        log("sending error", req);
        ws.send(JSON.stringify(req));
      };

      if (obj.cmds !== undefined) {
        $.each(obj.cmds, function(i, v) {
          var val, h = config.hnd[v.cmd];
          res.id = v.id;
          delete res.resp;
          if (h) {
            try {
              var done = function(val, st) {
                var rk = "resp";
                res.id = v.id;
                res.status = st || 0;
                if (st !== undefined) {
                  rk = "status_msg";
                }
                res[rk] = val;
                if (val === undefined) {
                  delete res[rk];
                }
                log("sending", req);
                me._send(req);
              };

              if (h.length > 1) {
                h(v, done);
              } else {
                done(h(v));
              }
            } catch(e) {
              error(JSON.stringify(e));
            }
          } else {
            error("unknown command " + v.cmd);
          }
        });
      }

      if (obj.resp !== undefined) {
        $.each(obj.resp, function(i, v) {
          numKeys++;
          if (v.id in config.map) {
            config.map[v.id](v);
            delete config.map[v.id];
            numKeys--;
          }
        });
      }
      if (config.onstop && numKeys == 0) {
        config.onstop();
      }
      // TODO(lsm): cleanup old, stale callbacks from the map.
    };
  };

  reconnect();
};

/*
 * UBJSON disabled until we can request WS extensions to
 * the WebSocket API.
 */
/*
if (typeof UBJSON !== "undefined") {
  Clubby.prototype._send = function(req) {
    var ws = this.config.ws, first = true;
    UBJSON.render(req, function(b) {
      ws.send(new Blob(first ? [b, undefined] : [undefined,b,undefined]));
      first = false;
    }, function(e) {
      if (e !== undefined && this.config.log) {
        console.log("error rendering", e);
      }
      ws.send(new Blob([undefined, ""]));
    });
  }
} else {
  Clubby.prototype._send = function(req) {
    this.config.ws.send(JSON.stringify(req));
  }
}
*/
Clubby.prototype._send = function(req) {
  this.config.ws.send(JSON.stringify(req));
}

Clubby.prototype.call = function(dst, cmd, callback) {
  var c = this.config;
  var log = function(a,b) {
    if (c.log) {
      console.log(a, b);
    }
  };
  var id = c.next_req_id++;
  var req = { v: 1, dst: dst, src: c.src, key: c.key };
  req.cmds = [ $.extend({ id: id}, cmd) ];
  c.map[id] = callback; // Store callback for the given message ID
  var msg = JSON.stringify(req);
  if (c.onstart) {
    c.onstart(msg);
  }
  if (c.ws.readyState == WebSocket.OPEN) {
    log('call sending: ', msg);
    this._send(req);
  } else if (c.ws.readyState == WebSocket.CLOSED) {
    c.ws.close();
  } else {
    log('NOT sending:', c.ws.readyState, msg);
    if (c.onerror) {
      c.onerror(c.ws, msg);
    }
  }
};

Clubby.prototype.oncmd = function(cmd, cb) {
  this.config.hnd[cmd] = cb;
}

Clubby.prototype.ready = function(cb) {
  var c = this.config;
  if (c.ws.readyState == WebSocket.OPEN) {
    cb();
  } else {
    c.rdy.push(cb);
  }
}
