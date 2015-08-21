// compat
$ = {};
$.extend = function(a, b) {
    for(k in b) a[k] = b[k];
    return a;
};

$.each = function(a, f) {
    a.forEach(function(v, i) {f(i, v)});
};

console = {};
console.log = print;

File.eval("clubby.js");
clubby = new Clubby({url:"ws://api.cesanta.com:80", "src": conf.dev.id, "key": conf.dev.key})
