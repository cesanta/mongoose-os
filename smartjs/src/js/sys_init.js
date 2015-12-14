$ = {};
$.extend = function(a, b) {
  if(a === undefined) {
    a = {};
  }
  for (k in b) {
    if (typeof(a[k]) === 'object' && typeof(b[k]) == 'object') {
      $.extend(a[k], b[k]);
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

console = { log: print };

Sys.conf = File.loadJSON('conf_sys_defaults.json') || {};
$.extend(Sys.conf, File.loadJSON('conf.json') || {});
Sys.id = Sys.ro_vars.arch + '_' + (Sys.ro_vars.mac_address || '0');

print('\nStarting Smart.js - see documentation at',
      'https://cesanta.com/developer/smartjs',
      '\n==> Sys:\n', Sys, '\n');

File.eval('app_init.js');
