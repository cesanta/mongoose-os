// Copyright (c) 2015 Cesanta Software Limited
// All rights reserved

function ClubbyError(message, status, call) {
  this.name = "ClubbyError";
  this.message = call.method + ': '+ message;
  this.status = status;
  this.call = call;
  this.stack = (new Error()).stack;
}
ClubbyError.prototype = Object.create(Error.prototype);
ClubbyError.prototype.constructor = ClubbyError;

// I didn't figure out how to import jquery in es6+babel
// let's just make a shim of the couple of minor features we use;
// curtesy of fw/src/js/sys_init.js
if (typeof($) === 'undefined') {
  if (typeof(global) != 'undefined') {
    global.$ = {};
  } else {
    $ = {};
  }
  $.extend = function(deep, a, b) {
    if(typeof(deep) !== 'boolean') {
      b = a;
      a = deep;
      deep = false;
    }

    if(a === undefined) {
      a = {};
    }
    for (var k in b) {
      if (typeof(a[k]) === 'object' && typeof(b[k]) === 'object') {
        $.extend(a[k], b[k]);
      } else if(deep && typeof(b[k]) === 'object') {
        a[k] = $.extend(undefined, b[k]);
      } else {
        a[k] = b[k];
      }
    }
    return a;
  };

  $.each = function(a, f) {
    a.forEach(function(v, i) {
      f(i, v);
    });
  };
}

var Clubby = function(arg) {
  // arg is a map:
  //  {
  //    url: URL_OF_API_BACKEND_OPTIONAL,
  //    src: CLIENT_ID,
  //    key: CLIENT_PASSWORD,
  //    timeout: timeout_in_seconds, // Default command timeout
  //    log: BOOLEAN,
  //    eh: EXTRA_HEADERS_STRING,    // Extra HTTP headers on WS negotiation
  //    ephemeral: BOOLEAN,          // if true do append random str to url, default true
  //    onopen: CALLBACK_OPTIONAL,   // Called when socket is connected
  //    onstart: CALLBACK_OPTIONAL,  // Called at any clubby call
  //    onstop: CALLBACK_OPTIONAL,   // Called when all clubby calls are ended
  //    oncmd: CALLBACK_OPTIONAL     // Called on each incoming command
  // }

  // This is a random component part which will be added to the
  // source for all outgoing commands, to make this client unique.
  var randomString = '.' + Math.random().toString(36).substring(7);
  var defaultUrl = 'wss://api.cesanta.com:443';

  var config = this.config = $.extend(arg, {
    next_req_id: 1, // Auto-incrementing command ID
    map: {},        // request_id -> callback mapping
    hnd: {},        // command_id -> callback mapping
    rdy: [],        // callbacks called once as soon client is connected
    queue: [],      // Requests to send as soon client is connected
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
    var ws = config.ws = new WebSocket(url, 'clubby.cesanta.com', config.eh);

    ws.onclose = function() {
      log('ws closed');
      // Schedule a reconnect after 1 second
      if (config.onclose) config.onclose();
      setTimeout(reconnect, 1000);
    };

    ws.onopen = function() {
      log('connected');
      if (config.onopen) config.onopen();
      $.each(config.rdy, function(i, r) { r() });
      config.rdy = [];
      $.each(config.queue, function(i, req) { me._send(req); });
      config.queue = [];
    };

    // This is a non-standard callback.
    // Receives number of bytes still waiting to be sent to the client.
    ws.onsend = config.onsend;

    ws.onmessage = function(ev) {
      // Dispatch responses to the correct callback
      log('received: ', ev.data);
      var frame;
      try {
        frame = JSON.parse(ev.data);
      } catch(e) {
        log('bad frame:', ev.data, e);
        return;
      }

      if (frame.method !== undefined) {
        var cmd = {id: frame.id, cmd: frame.method, args: frame.args};
        setTimeout(function() { handleReq(cmd, frame.src) }, 0);
      } else if (frame.id in config.map) {
        var resp = {id: frame.id};
        if (frame.error) {
          resp.status = frame.error.code;
          resp.status_msg = frame.error.message;
        } else {
          resp.status = 0;
        }
        if (frame.result !== undefined) resp.resp = frame.result;
        config.map[frame.id](resp);
        delete config.map[frame.id];
      }
      if (config.onstop && $.isEmptyObject(config.map)) {
        config.onstop();
      }
      // TODO(lsm): cleanup old, stale callbacks from the map.
    };

    function handleReq(cmd, src) {
      log('handling', cmd);
      var h = config.hnd[cmd.cmd];
      var frame = {v: 2, dst: src, src: config.src, key: config.key, id: cmd.id};

      var error = function(e) {
        frame.error = {code: 1, message: e};
        log("sending error", frame);
        ws.send(JSON.stringify(frame));
      };

      if (h) {
        var done = function(val, st) {
          if (val !== undefined) frame.result = val;
          if (st !== undefined) {
            if (typeof st == "object") {
              frame.error = st;
            } else {
              frame.error = {code: st, message: String(val)};
            }
          }
          me._send(frame);
        };

        if (h.length > 1) {
          setTimeout(function(){
            try {
              h(cmd, done);
            } catch(e) {
              error(JSON.stringify(e));
            }
          }, 0);
        } else {
          setTimeout(function(){
            try {
              Promise.resolve(h(cmd)).then(done, function(e) { done(e, 1); });
            } catch(e) {
              error(JSON.stringify(e));
            }
          }, 0);
        }
      } else {
        error("unknown command " + cmd.cmd);
      }
    }
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
  if (this.config.log) console.log('sending: ', req);
  this.config.ws.send(JSON.stringify(req));
};

Clubby.prototype.call = function(method, args, opts) {
  var callback;

  if (typeof opts == 'function') {
    callback = opts;
  } else {
    opts = opts || {};
    callback = opts.cb; // user callback, if empty return a promise
  }

  var cb; // internal callback
  var p;
  if (callback === undefined) {
    p = new Promise(function(resolve, reject) {
      cb = function cb(req) {
        if (req.status == 0) {
          resolve(req.resp);
        } else {
          reject(new ClubbyError(req.status_msg, req.status, {
            method: method,
            args: args,
            opts: opts
          }));
        }
      };
    });
  } else {
    cb = callback;
  }

  var c = this.config;
  var id = c.next_req_id++;
  var req = {
      v: 2, dst: opts.dst, src: c.src, key: c.key,
      id: id, method: method, args: args,
      deadline: opts.deadline, timeout: opts.timeout
  };
  if (req.timeout === undefined) req.timeout = c.timeout;
  c.map[id] = cb; // Store callback for the given message ID
  var msg = JSON.stringify(req);
  if (c.onstart) {
    c.onstart(msg);
  }
  if (c.ws.readyState == WebSocket.OPEN) {
    this._send(req);
  } else {
    if (this.config.log) console.log('queuing: ', req);
    c.queue.push(req);
  }

  return p;
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

if (typeof(module) !== 'undefined') {
  module.exports.Clubby = Clubby;
  module.exports.ClubbyError = ClubbyError;
}
