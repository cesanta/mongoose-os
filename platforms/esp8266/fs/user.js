// HW setup:
//   GPIO 12, 14 - I2C SDA, SCL. Attached:
//     Midas MCXXX 2x16 LCD @ 0x3E, reset connected to GPIO 0
//     ADPC9301 ambient light sensor @ 0x39 (addr sel pin floating).
//   GPIO 10 - LED
//   GPIO 13 - LCD backlight (PWM controlled)
//

File.eval("I2C.js");
File.eval("ADPS9301.js");
File.eval("MCXXX_LCD.js");
File.eval("clubby.js");

print("Initializing Ponsacco project, please stand by...");

//var WIFI_SSID = 'TehCloud';
//var WIFI_PASS = '';
//var WIFI_SSID = 'Over_9000_Internets';
//var WIFI_PASS = 'Over_9000_Internets';
var WIFI_SSID = 'Cesanta';
var WIFI_PASS = 'EmbeddedCommunication';

var clubby;
var ls0 = -1, ls1 = -1;

Debug.mode(1);

// Set up LED.
function LED(pin) {
  var _v;
  this.set = function(v) {
    if (typeof v !== "number" && typeof v !== "boolean") {
      throw("invalid value type: " + (typeof v));
    }
    if (v < 0 || v > 1) throw("value out of range");
    if (!PWM.set(10, 10000, 10000 * v)) throw("failed to configure LED");
    _v = v;
  }

  this.get = function() {
    return _v;
  }
}
var led = new LED(10);
led.set(0);

var i2c = new I2C(12, 14);

if (0) {
print("Initializing LCD...");
var lcd = new MCXXX(i2c, 2 /* lines */, 16 /* columns */);
if (!lcd.init(true /* display on */, true /* cursor on */, true /* blinking */,
              2 /* contrast */)) {
  print("failed to init LCD!");
  OS.reset();
}
GC.gc();
if (!lcd.setText("Welcome to\n      Ponsacco!")) {
  print("failed to set LCD text!");
  OS.reset();
}
GC.gc();

print("Initializing light sensor...");
var ls = new ADPS9301(i2c, 2 /* addr_sel floating */);
if (!ls.setPower(true /* on */) ||
    !ls.setOpts(true /* high gain */, 0 /* 13 ms integrations. */)) {
  print("failed to init light sensor!");
  OS.reset();
}

print("Starting backlight regulation...");

function do_ls(ls) {
  GC.gc();
  ls0 = ls.readData(0);
  ls1 = ls.readData(1);
  if (ls0 >= 0) {
    var pwm = Math.min(10000, 200 + ls0 * 10);
    PWM.set(13, 10000, pwm);
//    print(ls0, ls1, pwm, OS.prof());
  }
//  setTimeout(function() { do_ls(ls); }, 1000);
}

do_ls(ls);
GC.gc();
}

print("Connecting to WiFi network " + WIFI_SSID + "...");

function clubbySend() {
  var cmd = { cmd: '/v1/Metrics.Publish' };
  cmd.args = {};
  cmd.args.vars = [];
  if (ls0 >= 0) cmd.args.vars.push([{"__name__": "light", "sensor": "0"}, ls0]);
  if (ls1 >= 0) cmd.args.vars.push([{"__name__": "light", "sensor": "1"}, ls1]);
  ls0 = ls1 = -1; /* Only report same values once. */
  clubby.call('//api.cesanta.com', cmd, function(a) {
    print('done:', a);
    //setTimeout(clubbySend, 1000);
  });
  GC.gc();
}

function clubbyConnected() {
  print("Clubby connected");
  clubby.oncmd('/v1/LED.Set', function(cmd, cb) {
    led.set(cmd.args.value);  // Throws on error.
    cb(undefined, 0);
  });
  clubby.oncmd('/v1/LED.Get', function(cmd, cb) {
    cb({"value": led.get()});
  });
  clubby.oncmd('/v1/LCD.SetText', function(cmd, cb) {
    if (!lcd.setText(cmd.args.text)) {
      cb("failed to set LCD text", 1);
    }
    cb("ok", 0);
  });
  setTimeout(clubbySend, 100);
}

function do_clubby() {
/*  var ccfg = {
    url: 'ws:' + conf.cloud + ":80",
    src: conf.dev.id,
    key: conf.dev.key,
    log: true,
  }; */
/*
  var ccfg = {
    url: 'ws://api.cesanta.com:80',
    src: '//api.cesanta.com/d/dev_14797',
    key: 'psk_83192',
    log: true
  };
*/
  var ccfg = {
    url: 'ws://demo.cesanta.com:8911',
//    url: 'ws://127.0.0.1:8911',
    src: '//api.cesanta.com/d/demo',
    key: 'demo',
    log: true
  };
  print("Connecting to", ccfg);
  ccfg.onopen = clubbyConnected;
  clubby = new Clubby(ccfg);
}

function Wifi_cb(ev) {
  if (ev != 2) return;
  print("Connected to WiFi, IP", Wifi.ip());
  setTimeout(do_clubby, 1000);
}

Wifi.changed(Wifi_cb);
Wifi.setup(WIFI_SSID, WIFI_PASS);
// cl.call('//api.cesanta.com', {cmd: '/v1/Metrics.foo', args: {"vars": [[{"__name__": "temp", "sensor": "1"}, 25]]} }, function(a) { print('done:', a) })

print("Init done.");

