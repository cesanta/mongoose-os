"use strict";
global.$ = {};
$.extend = function(deep, a, b) {
  if(typeof(deep) != 'boolean') {
    b = a;
    a = deep;
    deep = false;
  }

  if(a === undefined) {
    a = {};
  }
  for (var k in b) {
    if (typeof(a[k]) === 'object' && typeof(b[k]) == 'object') {
      $.extend(a[k], b[k]);
    } else if(deep && typeof(b[k]) == 'object') {
      a[k] = $.extend(undefined, b[k]);
    } else {
      a[k] = b[k];
    }
  }
  return a;
};

$.each = function(a, f) {
  a.forEach(function(v, i) {
    f(i, v)
  });
};

global.console = { log: print };

Sys.conf = File.loadJSON('conf_sys_defaults.json') || {};
$.extend(Sys.conf, File.loadJSON('conf.json') || {});
Sys.id = Sys.ro_vars.arch + '_' + (Sys.ro_vars.mac_address || '0');

global.confLoaded = JSON.parse(JSON.stringify(Sys.conf))

Sys.conf.save = function(reboot) {
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

  var newCfg = JSON.parse(JSON.stringify(Sys.conf))
  delete newCfg.save
  deleteUnchanged(newCfg, confLoaded);
  newCfg = $.extend(File.loadJSON('conf.json') || {}, newCfg);

  var cfgFile = File.open("conf.json.tmp", "w")
  cfgFile.write(JSON.stringify(newCfg))
  cfgFile.close()

  File.remove("conf.json");
  File.rename("conf.json.tmp", "conf.json");

  if (reboot != false) {
    Sys.reboot()
  }
}

print('\nStarting Smart.js - see documentation at',
      'https://cesanta.com/developer/smartjs',
      '\n==> Sys:\n', Sys, '\n');

print('Device id:', Sys.conf.clubby.device_id)
print('Device psk:', Sys.conf.clubby.device_psk, '\n')

global.clubby = new Clubby({connect:false});

if (Sys.conf.clubby.connect_on_boot) {
  if (typeof(Wifi) != "undefined") {
    Wifi.ready(clubby.connect.bind(clubby))
  } else {
    // Assume, if port doesn't have Wifi object it uses
    // external networking and if connect_on_boot=true we have
    // connect immediately
    clubby.connect()
  }
}

File.eval('app_init.js');
