// Smartjs runtime

// polyfills
$ = {};
$.extend = function(a, b) {
  for (k in b) a[k] = b[k];
  return a;
};

$.each = function(a, f) {
  a.forEach(function(v, i) {
    f(i, v)
  });
};

console = {
  log: print
};

// Runtime functions after init
SJS = {};
SJS.init = function() {
  conf.save = function() {
    var f = File.open("sys_config.json", "w");
    if (f) {
      f.write(JSON.stringify(conf));
      f.close();
    } else {
      print("cannot save conf");
    }
  }

  if (typeof Wifi !== undefined) {
    Wifi.changed(function(e) {
      switch (e) {
        case 0: print("Wifi disconnected"); break;
        case 2: print("Wifi connected, IP:", Wifi.ip()); break;
      };
    });

    if (conf.wifi) {
      if (conf.wifi.ssid) {
        Wifi.setup(conf.wifi.ssid, (conf.wifi.known || {})[conf.wifi.ssid] || '');
      } else {
        print("Scanning nets");
        Wifi.scan(function(l) {
          for(var i in l) {
            var n = l[i];
            if (n in conf.wifi.known) {
              print("Joining", n);
              Wifi.setup(n, conf.wifi.known[n]);
              return;
            }
          }
          print("Cannot find known network, use Wifi.setup");
        });
      }
    }
  }
};

// cloud
function initCloud(ll) {
  File.eval("clubby.js");
  clubby = new Clubby({
    url: 'ws:' + conf.cloud + ':80',
    src: conf.dev.id,
    key: conf.dev.key,
    log: !!ll,
    onopen: function() {
      clubby.call(conf.cloud, {cmd: "/v1/Hello"}, function() {});
    }
  });
  File.eval("swupdate.js");
  SWUpdate(clubby);
  File.eval("fileservice.js");
  FileService(clubby);
}

