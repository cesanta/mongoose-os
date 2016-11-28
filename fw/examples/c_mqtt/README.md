# Simple device control using MQTT

See [quick start guide](https://mongoose-iot.com/docs/#/quickstart/)
on how to build and flash the firmware.


## To connect Amazon IoT and test the example with its MQTT Broker do:
First, download the Amazon's `aws` utility and run `aws configure`.
Then you're ready to onboard

```
miot build
miot flash
miot aws-iot-setup
```

Login to AWS IoT console, send `{pin: 2, state: 0}` message to the device's
topic.
