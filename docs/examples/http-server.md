---
title: HTTP server example
---

This example shows how to start an HTTP server, serve an HTML form that allows
to write 0 or 1 to any GPIO pin.

- Store this JavaScript snippet into a file `index.html`

```html
<html>
  <h1>Hello from ESP8266</h1>
  <form method="POST" action="/gpio">
    GPIO: <input type="text" name="pin" size="2" />
    Value:
    <select name="value">
      <option>0</option>
      <option>1</option>
    </select>
    <input type="submit" value="Set" />
  </form>
</html>
```

- Store this JavaScript snippet into a file `http_server_1.js`

```javascript
var parse = function(query) {
  var obj = {};
  var pairs = query.split('&');
  for (var i = 0; i < pairs.length; i++) {
    var x = pairs[i].split('=', 2);
    obj[x[0]] = x[1];
  }
  return obj;
}

var server = Http.createServer(function(req, res) {
  print(req);
  if (req.url == '/gpio') {
    var query = parse(req.body);
    var pin = parseInt(query.pin), value = parseInt(query.value);
    GPIO.setmode(pin, 0, 0);
    GPIO.write(pin, value);
    res.writeHead(302, {'Location': '/'});
    res.end();
  } else {
    res.serve();
  }
}).listen(8080);

```

- Upload files to the device: start FlashNChips, go to File/Upload, choose
  `http_server_1.js` then `index.html`
- In the FlashNChips console, type `Wifi.setup('SSID', 'PASSWORD')`. Use SSID
  and password of your WiFi network.
- After couple of seconds, type `Wifi.ip()`. Notice the IP address given.
- InIn the FlashNChips console, type `File.eval('http_server_1.js')`.
- Start web browser, point to the module's IP address and port 8080, eg:
  192.168.1.148:8080

<img src="http_server_1.png" align="center"/>

- Attach LED to GPIO pin 5
- Choose pin 5 number and value 1, press Set
- Notice the LED is set

<img src="http_server_2.png" align="center"/>

- Notice HTTP server messages in the console log

<img src="http_server_3.png" align="center"/>

- Note: if you want to create a server on port 80, you have to make sure the
  config web server is off (by unchecking the `Enable HTTP Server` config
  setting) or moved to another port.  You can do this from the serial promp
  itself with:

```javascript
Sys.conf.http.enable = false;
Sys.conf.save(true);
```
