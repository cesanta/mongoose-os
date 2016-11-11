# Remote device control using MQTT protocol

See instructions at https://mongoose-iot.com/docs/#/quickstart/remote-control.md/
By default, Mongoose Firmware uses Mongoose Cloud MQTT server running at mongoose.cloud:1883 . That MQTT server requires authentication, meaning you have to have an registerd account at http://mongoose.cloud and thus only you can talk to your devices. To use different MQTT server, do:

```
miot config-set -port /dev/ttyUSB0 mqtt.broker=HOST:PORT
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
