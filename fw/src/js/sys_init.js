"use strict";
global.$ = {};
$.extend = function(deep, a, b) {
  if(typeof(deep) !== 'boolean') {
    b = a;
    a = deep;
    deep = false;
  }

  if(a === undefined) {
    a = {};
  }
  for (var k in b) {
    if (typeof(a[k]) === 'object' && typeof(b[k]) === 'object') {
      $.extend(deep, a[k], b[k]);
    } else if(deep && typeof(b[k]) === 'object') {
      a[k] = $.extend(deep, undefined, b[k]);
    } else {
      a[k] = b[k];
    }
  }
  return a;
};

$.each = function(a, f) {
  a.forEach(function(v, i) {
    f(i, v);
  });
};

function loadCfg() {
  var ret = File.loadJSON('conf_sys_defaults.json');
  if(ret == undefined) {
    print("Failed to load configuration from conf_sys_defaults.json");
    return {};
  } else {
    $.extend(ret, File.loadJSON('conf.json'));
    return ret;
  }
}

Sys.conf = loadCfg();

Object.defineProperty(Sys.conf, "save", {
  value: function(reboot) {
    var deleteUnchanged= function(c1, c2) {
      var n = 0;
      for (var k in c1) {
        if (typeof(c1[k]) == 'object') {
          if (typeof(c2[k]) == 'undefined') { c2[k] = {} }
          var j = deleteUnchanged(c1[k], c2[k]);
          if (j == 0) { delete c1[k]; } else { n += j };
        } else {
          if (c1[k] == c2[k]) { delete c1[k]; } else { n++; };
        }
      }

      return n;
    };

    var newCfg = $.extend(true, undefined, Sys.conf);
    delete newCfg.save;
    deleteUnchanged(newCfg, loadCfg());

    newCfg = $.extend(File.loadJSON('conf.json') || {}, newCfg);

    var cfgFile = File.open("conf.json.tmp", "w");
    cfgFile.write(JSON.stringify(newCfg));
    cfgFile.close();

    File.remove("conf.json");
    File.rename("conf.json.tmp", "conf.json");

    if (reboot != false) {
      Sys.reboot();
    }
  }});

print('\nMongoose IoT Firmware - see documentation at',
      'https://cesanta.com/developer/fw',
      '\n==> Sys:\n', Sys, '\n');

if (Sys.conf.clubby.device_id) {
    print('Device credentials: ', {
      device_psk: Sys.conf.clubby.device_psk,
      device_id: Sys.conf.clubby.device_id,
    }, '\n');
}

global.clubby = new Clubby({connect:false});

if (typeof console !== "undefined") console.setClubby(clubby);

if (Sys.conf.clubby.device_id && Sys.conf.clubby.connect_on_boot) {
  if (Wifi.status() !== undefined) {
    // Wifi has some well-defined status; therefore, Wifi is usable at the
    // current platform
    Wifi.ready(clubby.connect.bind(clubby))
  } else {
    // Wifi management is not supported at the current platform: assume it uses
    // external networking and if connect_on_boot=true we can connect
    // immediately
    clubby.connect()
  }
}

if (File.exists('app.js')) {
  File.eval('app.js');
} else if (File.exists('demo.js')) {
  print('No app.js, running demo...');
  File.eval('demo.js');
}
