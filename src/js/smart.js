// compat
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

// TODO(lsm): auto-generate config file if it's not present,
// and remove this snippet.
if (typeof(conf) == 'undefined') {
  conf = {
    dev: {
      id: '//api.cesanta.com/d/dev_' + Math.random().toString(36).substring(7),
      key: 'psk_' + Math.random().toString(36).substring(7)
    }
  };
}

if (typeof(conf.dev.cloud) === 'undefined') {
  conf.dev.cloud = "//api.cesanta.com";
}

print('Device id: ' + conf.dev.id);
print('Device psk: ' + conf.dev.key);
print('Cloud: ' + conf.dev.cloud);

File.eval("clubby.js");
clubby = new Clubby({
  url: "ws:"+conf.dev.cloud+":80",
  src: conf.dev.id,
  key: conf.dev.key,
  log: false,
  onopen: function() {
    clubby.call(conf.dev.cloud, {cmd: "/v1/Hello"}, function() {});
  }
});
File.eval("swupdate.js");
SWUpdate(clubby);
File.eval("fileservice.js");
FileService(clubby);

