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
print('Device id: ' + conf.dev.id);
print('Device psk: ' + conf.dev.key);


File.eval("clubby.js");
clubby = new Clubby({
  url: "ws://api.cesanta.com:80",
  src: conf.dev.id,
  key: conf.dev.key,
  log: false,
  onopen: function() {
    clubby.call("//api.cesanta.com", {cmd: "/v1/Hello"}, function() {});
  }
});
File.eval("swupdate.js");
SWUpdate(clubby);
File.eval("fileservice.js");
FileService(clubby);

