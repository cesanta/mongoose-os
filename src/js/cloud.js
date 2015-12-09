var Cloud = {};
Cloud.mkreq = function(cmd, args, dst) {
    var ret = {v:1, src: Sys.conf.dev.id, dst: dst || "//api.cesanta.com", key: Sys.conf.dev.key, cmds:[{cmd:cmd, args: args}]};
    return JSON.stringify(ret);
}
Cloud.store = function(name,val,opts) {
    opts = opts || {};
    var b = opts.labels || {};
    b.__name__ = name;
    var args = {"vars": [[b, val]]};
    var d = this.mkreq("/v1/Metrics.Publish", args);
    delete b.__name__;
    args = b = null;
    if (typeof(Http.request) == 'undefined') {
      Http.post("http://api.cesanta.com", d, opts.cb || function() {});
    } else {
      Http.request({hostname:"api.cesanta.com", method: "POST"}, opts.cb || function() {}).end(d);
    }
}

Cloud.init = function(backend, device_id, device_psk) {
  File.eval("clubby.js");
  global.clubby = new Clubby({
    url: 'ws:' + backend + ':80',
    src: device_id,
    key: device_psk,
    log: false,
    onopen: function() {
      clubby.call(backend, {cmd: "/v1/Hello"}, function() {});
    }
  });
  File.eval("swupdate.js");
  SWUpdate(clubby);
  File.eval("fileservice.js");
  FileService(clubby);
}
