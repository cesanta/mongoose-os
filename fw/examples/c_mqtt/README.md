This is a C-only Mongoose IoT application.

Building requires [Docker](https://docs.docker.com/engine/installation/) and
 [miot tool](https://mongoose-iot.com/software.html).

Compile with `PLATFORM` set:

```
make PLATFORM=esp8266
```
or

```
make PLATFORM=cc3200
```

Firmware files will be stored in the `firmware` directory.

To flash the firmware you can use [Mongoose Flashing Tool](https://github.com/cesanta/mft):

```
  MFT --platform=esp8266 --port=/dev/ttyUSB0 --flash-baud-rate=1500000 --flash firmware/*-last.zip --console
```
or
```
  MFT --platform=cc3200 --port=/dev/ttyUSB1 --flash firmware/*-last.zip --console
```

To configure network use

```
miot dev config set -port /dev/ttyPORT wifi.ap.enable=false wifi.sta.enable=true wifi.sta.ssid=MYNET wifi.sta.pass=MYPASS
```

To set MQTT broker use

```
miot dev config set -port /dev/ttyPORT mqtt.broker=ADDRESS:IP
```
