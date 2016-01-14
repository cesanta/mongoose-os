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

print('\nStarting Smart.js - see documentation at',
      'https://cesanta.com/developer/smartjs',
      '\n==> Sys:\n', Sys, '\n');

if (Sys.conf.clubby.connect_on_boot) {
  global.clubby = new Clubby();
}

File.eval('app_init.js');
