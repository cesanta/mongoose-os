// TODO(mkm): put some examples and
// print out the link to our docs
print("HELO! Type some JS. See https://github.com/cesanta/smart.js for more info.");

var x;
function testHttp(url) {
    Http.get(url || "http://api.cesanta.com:80", function(d, e) { x=d; print("http gave data: ", d, " err: ", e); });
}

/* uncomment this to try out the GDB server */
/* crash(); */
