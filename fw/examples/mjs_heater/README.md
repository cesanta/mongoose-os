# Smart heater example

This is an example of the smart heater we use in our office,
implemented totally in JavaScript on top of vanilla mOS firmware,
[mjs\_base](https://github.com/cesanta/mongoose-os/tree/master/fw/examples/mjs_base)

It provides
a number of HTTP endpoints to get heater status and perform a remote control:

- `/` : Shows a web page with current status
- `/heater/on` : Turns a heater on
- `/heater/off` : Turns a heater off
- `/heater/status` : Shows current temperature and free RAM

A heater is equipped with a
[MCP9808](http://www.microchip.com/wwwproducts/en/en556182) temperature sensor.

This example reads temperature values at regular intervals and sends
them to the configured MQTT server / topic. You can use MQTT client console
at http://mqtt-helper.mybluemix.net/ to connect to MQTT server and
see published messages:

![Screenshot](screenshot.png?raw=true)

## Build instructions

```bash
mos flash --firmware https://mongoose-os.com/downloads/mos-esp8266.zip
mos put index.html
mos put init.js
mos config-set i2c.enable=true mqtt.server=test.mosca.io:1883
mos config-set wifi.sta.ssid=WIFI_SSID wifi.sta.pass=WIFI_PASS
mos config-set wifi.sta.enable=true wifi.ap.enable=false
mos console
```
