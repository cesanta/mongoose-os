# Simple device control using MQTT

See [quick start guide](https://mongoose-iot.com/docs/#/quickstart/)
on how to build and flash the firmware.


## To connect Amazon IoT and test the example with its MQTT Broker do:
First, download the Amazon's `aws` utility and run `aws configure`.
Then you're ready to onboard

```
miot build
miot flash
miot aws-iot-setup --aws-iot-policy YOUR_POLICY
miot config-set mqtt.pub=aa mqtt.sub=bb
miot config-set wifi.ap.enable=false wifi.sta.enable=true
miot config-set wifi.sta.ssid=YOUR_WIFI_NET wifi.sta.pass=YOUR_WIFI_PASSWORD
miot console
```

Login to AWS IoT console, send `{pin: 2, state: 0}` message to MQTT topic `bb`.
