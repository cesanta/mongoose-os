print("HELO! Type some JS. See https://github.com/cesanta/smart.js for more info.");

/* uncomment this to try out the GDB server */
/* crash(); */

/* TODO(mkm): move to 'smart.js' */
//File.load('smart.js');

var conf = {};
var cfile = File.open('config.json');
if (cfile != null) {
    conf = JSON.parse(cfile.readAll());
    if (conf.wifi) {
        if (conf.wifi.ssid) {
            Wifi.setup(conf.wifi.ssid, (conf.wifi.known || {})[conf.wifi.ssid] || '');
        } else {
            print("Scanning nets");
            Wifi.scan(function(l) {
                for(var i in l) {
                    var n = l[i];
                    if (n in conf.wifi.known) {
                        print("Joining ", n);
                        Wifi.setup(n, conf.wifi.known[n]);
                        return;
                    }
                }
                print("Cannot find known network, use Wifi.setup");
            });
        }
    }
    cfile.close();
}
delete cfile;

Cloud = {};
Cloud.mkreq = function(src,dst, key, cmd, args) {
    return JSON.stringify({v:1, src: src, dst: dst, key: key, cmds:[{cmd:cmd, args: args}]});
}
Cloud.store = function(name,val,opts) {
    opts = opts || {};
    var b = opts.labels || {};
    b.__name__ = name;
    var args = {"vars": [[b, val]]};
    var d = this.mkreq(conf.id, "//api.cesanta.com", conf.key, "/v1/Metrics.Publish", args);
    delete b.__name__;
    args = b = null;
    Http.post("http://api.cesanta.com:80", d, opts.cb || function() {});
}

File.load('MCP9808.js');
t = new MCP9808(14,12,1,1,1);

function demo(n, cb) { Cloud.store("temperature",n,{labels: {"sensor":"1"},cb:cb}) }
function temp() { demo(t.getTemp(), function (d) { print("got:", d, ",mem:", GC.stat().sysfree); setTimeout(temp, 2000) }) };
