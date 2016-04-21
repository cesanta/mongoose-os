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

Sys.conf = File.loadJSON('conf_sys_defaults.json') || {};
$.extend(Sys.conf, File.loadJSON('conf.json') || {});

Object.defineProperty(Sys, "oconf", { value: JSON.parse(JSON.stringify(Sys.conf))});

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

    var newCfg = JSON.parse(JSON.stringify(Sys.conf));
    delete newCfg.save;
    deleteUnchanged(newCfg, Sys.oconf);
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

print('Sys:\n', Sys, '\n');

if (File.exists('main.js')) {
  File.eval('main.js');
}
