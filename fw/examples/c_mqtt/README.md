# Simple device control using MQTT

See [quick start guide](https://mongoose-iot.com/docs/#/quickstart/overview.md/)
on how to build and flash the firmware, and follow to the

This example shows how to control a device remotely using MQTT protocol.
We're going to use [Mongoose Cloud](http://mongoose.cloud),
which provides authenticated MQTT service. Any other public or private MQTT
server would also work.

```bash
git clone https://github.com/cesanta/mongoose-iot
cd mongoose-iot/fw/examples/c_mqtt
miot build --arch ARCHITECTURE # cc3200 or esp8266
miot flash
miot register
miot miot config-set wifi.ap.enable=false wifi.sta.enable=true \
  wifi.sta.ssid=YOUR_WIFI_NETWORK wifi.sta.pass=YOUR_WIFI_PASSWORD
miot console


ev_handler           3355550 MQTT Connect (1)
ev_handler             53003 CONNACK: 0
sub                     1376 Subscribed to /DEVICE_ID/gpio
```

The `miot console` command attaches to the device's UART and prints log
messages. Device connects to the cloud, subscribes to the MQTT topic
`/device_id/gpio` and expects messages like
`{"pin": NUMBER, "state": 0_or_1}` to be sent to that topic.
Device sets the specified GPIO pin to 0 or 1, demonstrating remote control
capability.

In a separate terminal, send such control message using mosquitto command line
tool:

```
mosquitto_pub -h mongoose.cloud -u YOUR_USER -P YOUR_PASSWORD \
  -t /DEVICE_ID/gpio -m '{pin:14, state:1}'
```
The device sets the GPIO pin:

```
ev_handler           248753866 Done: [{pin:14, state:1}]
```

If an LED is attached to that pin, it'll turn on. Note that the whole
transaction is authenticated: only the owner of the device (you) can operate it.

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
