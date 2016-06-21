---
title: JS example - HTTP server
---

This example shows how to start an HTTP server, serve an HTML form that allows
to write 0 or 1 to any GPIO pin.

- Login to the cloud IDE
- Copy/paste the following code into the file `gpio.html`

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

- Copy/paste the following code into the file `app.js`

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
    GPIO.setMode(pin, 0, 0);
    GPIO.write(pin, value);
    res.writeHead(302, {'Location': '/'});
    res.end();
  } else {
    res.serve();
  }
}).listen(8080);

```

- Attach an LED to GPIO 2 and GND pins
- Build and Flash firmware to the device
- Start the web browser, point to the device's IP address and port 8080, eg:
  192.168.1.148:8080

<img src="media/http_server_1.png" align="center"/>

- Attach LED to GPIO pin 5
- Choose pin 5 number and value 1, press Set
- Notice the LED is set
- Notice HTTP server messages in the console log
- Note: if you want to create a server on port 80, you have to make sure the
  system web server is off (by unchecking the `Enable HTTP Server` config
  setting) or moved to another port.  You can do this from the serial promp
  itself with:

```javascript
Sys.conf.http.enable = false;
Sys.conf.save(true);
```
