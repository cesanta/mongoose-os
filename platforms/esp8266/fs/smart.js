print("\nMy Cesanta cloud ID:", conf.dev.id);
print("My PSK:", conf.dev.key);
print("Use these credentials to add this device on https://dashboard.cesanta.com/");

conf.save = function() {
    var s = function(n,t) {
        var f = File.open(n, "w");
        if (f) {
            f.write(JSON.stringify(t));
            f.close();
        }
    };
    s("sys.json", this.sys); s("user.json", this.user);
};

if (conf.sys.wifi) {
    if (conf.sys.wifi.ssid) {
        Wifi.setup(conf.sys.wifi.ssid, (conf.sys.wifi.known || {})[conf.sys.wifi.ssid] || '');
    } else {
        print("Scanning nets");
        Wifi.scan(function(l) {
            for(var i in l) {
                var n = l[i];
                if (n in conf.sys.wifi.known) {
                    print("Joining", n);
                    Wifi.setup(n, conf.sys.wifi.known[n]);
                    return;
                }
            }
            print("Cannot find known network, use Wifi.setup");
        });
    }
}

Cloud = {};
Cloud.mkreq = function(cmd, args, dst) {
    return JSON.stringify({v:1, src: conf.dev.id, dst: dst || "//api.cesanta.com", key: conf.dev.key, cmds:[{cmd:cmd, args: args}]});
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

/* demo */
if (conf.user.demo === undefined || conf.user.demo == 'MCP9808') {
    File.eval('I2C.js');
    File.eval('MCP9808.js');
    t = new MCP9808(new I2C(14,12), MCP9808.addr(1,1,1));
    if (!conf.has_temp_sensor) {
        t.getTemp = function() { return 20+Math.random()*20 };
    }
}
if (conf.user.demo == 'MC24FC') {
    File.eval('I2C.js');
    File.eval('MC24FC.js');
    File.eval('MC24FC_test.js');
}

function push(n, cb) {
    Cloud.store("temperature", n, {labels: {"sensor": "1"}, cb: cb});
}

function demo() {
    Debug.print("Pushing metric")
    push(t.getTemp(), function(d, e) {
        if (e) {
            Debug.print("Error sending to the cloud:", e);
        } else {
            Debug.print("Cloud reply:", d);
        }
        setTimeout(demo, 2000)});
};

function startDemo() {
    print("Starting temperature push demo in background, type Debug.mode(1) to see activity");
    demo();
}

/* put here code that reacts to networking changes */
Wifi.changed(function(e) {
    /* got ip */
    if (e == 3) {
        if (conf.user.demo == 'MCP9808') {
            startDemo();
        }
        Wifi.changed(undefined);
    }
});
