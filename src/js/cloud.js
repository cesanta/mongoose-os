Cloud = {};
Cloud.mkreq = function(cmd, args, dst) {
    var ret = {v:1, src: conf.dev.id, dst: dst || "//api.cesanta.com", key: conf.dev.key, cmds:[{cmd:cmd, args: args}]};
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
    Http.post("http://api.cesanta.com", d, opts.cb || function() {});
}
