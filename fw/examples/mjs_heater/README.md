# Smart heater example

This is an example of the smart heater we use in our office,
implemented totally in JavaScript on top of vanilla mOS firmware,
https://github.com/cesanta/mongoose-os/tree/master/fw/examples/mjs_hello

## Build instructions

```bash
mos flash --firmware https://mongoose-iot.com/downloads/fw-esp8266.zip
mos put index.html
mos put init.js
mos config-set wifi.sta.enable wifi.sta.ssid=WIFI_SSID wifi.sta.pass=WIFI_PASS
mos console
```
