print("HELO! Type some JS. See https://github.com/cesanta/smart.js for more info.");

/* uncomment this to try out the GDB server */
/* crash(); */

/* TODO(mkm): move to 'smart.js' */
//File.load('smart.js');

if (conf.sys.wifi) {
    if (conf.sys.wifi.ssid) {
        Wifi.setup(conf.sys.wifi.ssid, (conf.sys.wifi.known || {})[conf.sys.wifi.ssid] || '');
    } else {
        print("Scanning nets");
        Wifi.scan(function(l) {
            for(var i in l) {
                var n = l[i];
                if (n in conf.sys.wifi.known) {
                    print("Joining ", n);
                    Wifi.setup(n, conf.sys.wifi.known[n]);
                    return;
                }
            }
            print("Cannot find known network, use Wifi.setup");
        });
    }
}

Cloud = {};
Cloud.mkreq = function(src,dst, key, cmd, args) {
    return JSON.stringify({v:1, src: src, dst: dst, key: key, cmds:[{cmd:cmd, args: args}]});
}
Cloud.store = function(name,val,opts) {
    opts = opts || {};
    var b = opts.labels || {};
    b.__name__ = name;
    var args = {"vars": [[b, val]]};
    var d = this.mkreq(conf.dev.id, "//api.cesanta.com", conf.dev.key, "/v1/Metrics.Publish", args);
    delete b.__name__;
    args = b = null;
    Http.post("http://api.cesanta.com:80", d, opts.cb || function() {});
}

/* demo */
if (conf.user.demo == 'MCP9808') {
    File.load('MCP9808.js');
    t = new MCP9808(14,12,1,1,1);
    if (!conf.has_temp_sensor) {
        t.getTemp = function() { return 20+Math.random()*20 };
    }
}
if (conf.user.demo == 'MC24FC') {
    File.load('MC24FC.js');
    File.load('MC24FC_test.js');
}

function push(n, cb) {
    Cloud.store("temperature", n, {labels: {"sensor": "1"}, cb: cb});
}

function demo() {
    push(t.getTemp(), function(d) {
        print("got:", d, ",mem:", GC.stat().sysfree);
        setTimeout(demo, 2000)});
};

/* put here code that reacts to networking changes */
Wifi.changed(function(e) {
    /* got ip */
    if (e == 3) {
        if (conf.user.demo == 'MCP9808') demo();
        Wifi.changed(undefined);
    }
});
