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
conf = File.loadJSON('sys_config.json');
if (typeof(conf) == 'undefined') {
  conf = {
    dev: {
      // TODO(lsm): refactor to initialize ID and PSK lazily on first
      // clubby request, get those from the cloud
      id: '//api.cesanta.com/d/dev_' + Math.random().toString(36).substring(7),
      key: 'psk_' + Math.random().toString(36).substring(7)
    },
    cloud: '//api.cesanta.com'
  };
  var f = File.open('sys_config.json', 'w');
  if (f) {
    f.write(JSON.stringify(conf));
    f.close();
  }
}

print('Device id: ' + conf.dev.id);
print('Device psk: ' + conf.dev.key);
print('Cloud: ' + conf.cloud);

function initCloud() {
  File.eval("clubby.js");
  clubby = new Clubby({
    url: 'ws:' + conf.cloud + ':80',
    src: conf.dev.id,
    key: conf.dev.key,
    log: false,
    onopen: function() {
      clubby.call(conf.cloud, {cmd: "/v1/Hello"}, function() {});
    }
  });
  File.eval("swupdate.js");
  SWUpdate(clubby);
  File.eval("fileservice.js");
  FileService(clubby);
}

