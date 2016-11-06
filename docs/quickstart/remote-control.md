---
title: Remote control - MQTT
---

This example shows how to control a device remotely using MQTT protocol.
We're going to use Mongoose Cloud, which provides authenticated
MQTT service - but any other public or private MQTT server would also work.

Assuming you have downloaded the
[`miot` utility](https://mongoose-iot.com/software.html) and have registered
an account at [Mongoose Cloud](http://cloud.mongoose-iot.com),
please follow these steps:

```bash
git clone https://github.com/cesanta/mongoose-iot
cd mongoose-iot/fw/examples/c_mqtt
miot build --arch ARCHITECTURE # cc3200 or esp8266
miot flash --port SERIAL_PORT
miot register --user YOUR_USER --pass YOUR_PASSWORD --port SERIAL_PORT
miot miot config-set --port SERIAL_PORT \
  wifi.ap.enable=false \
  wifi.sta.enable=true \
  wifi.sta.ssid=YOUR_WIFI_NETWORK \
  wifi.sta.pass=YOUR_WIFI_PASSWORD
miot console --port SERIAL_PORT


ev_handler           3355550 MQTT Connect (1)
ev_handler             53003 CONNACK: 0
sub                     1376 Subscribed to /DEVICE_ID/gpio
```

The last command attaches to the device's UART and prints log messages.
Device connects to the cloud, subscribes to the MQTT topic `/device_id/gpio`
and expects messages like `{"pin": NUMBER, "state": 0_or_1}` to be sent to
that topic. Device sets the specified GPIO pin to 0 or 1, demonstrating
remote control capability.

In a separate terminal, send such control message using mosquitto command
line tool:

```bash
mosquitto_pub -h cloud.mongoose-iot.com -u YOUR_USER -P YOUR_PASSWORD \
  -t /DEVICE_ID/gpio -m '{pin:14, state:1}'
```

The device sets the GPIO pin:

```
ev_handler           248753866 Done: [{pin:4, state:1}]
```

If an LED is attached to that pin, it'll turn on.

Note that the whole transaction is authenticated: only the owner of the
device (you) can operate it.
