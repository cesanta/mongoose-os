$ = {};
$.extend = function(a, b) {
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

console = {
  log: print
};

Sys.conf = File.loadJSON('conf_sys_defaults.json') || {};
$.extend(Sys.conf, File.loadJSON('conf.json') || {});

print('\nStarting Smart.js - see documentation at',
      'https://cesanta.com/developer/smartjs',
      '\n========== Sys.conf:\n', Sys.conf,
      '\n========== Sys.ro_vars:\n', Sys.ro_vars,
      '\n==========\n');

File.eval('app_init.js');
