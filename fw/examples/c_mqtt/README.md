# This is a C-only Mongoose IoT application.

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

## To connect Amazon IoT and test the example with its MQTT Broker do:
- In Amazon IoT Console create thing, attach certificate and policy to it.
- Download thing certificate, thing private key and CA certificate
- Find thing MQTT endpoint (in Amazon IoT console)
- Build the example
```
$ miot build --local --repo <path to cloned repo> --arch <esp8266 or cc3200>
```
- Flash the firmare
```
$ miot flash --port /dev/ttyUSB0
```
- Put downloaded certificates to the device
```
$ miot put --port /dev/ttyUSB0 ~/Downloads/mytestdevice.crt
$ miot put --port /dev/ttyUSB0 ~/Downloads/mytestdevice.key
$ miot put --port /dev/ttyUSB0 ~/Downloads/root-CA.crt
```
- Configure the device
```
$  miot config-set --port /dev/ttyUSB0 wifi.ap.enable=false wifi.sta.enable=true wifi.sta.ssid=<Your WiFi SSID> wifi.sta.pass=<Your WiFi password> mqtt.server=<your device MQTT endpoint>:8883 mqtt.ssl_cert=mytestdevice.crt mqtt.ssl_key=mytestdevice.key mqtt.ssl_ca_cert=root-CA.crt device.id="MyTestDevice"
```
- In AWS IoT Console switch to `MQTT Client` page and
  - press `Generate client ID`
  - press `Connect`
  - press `Publish to topic`
  - enter `/MyTestDevice/gpio` in `Publish topic`
  - enter `{pin: 5, state: 1}` in `Payload`
  - press `Publish`

The state of GPIO 5 will be changed to 1.
