---
title: AWS IoT integration
---

Mongoose Firmware is integrated natively with AWS IoT cloud.

AWS IoT uses MQTT for communication, so navigate to the
[c_mqtt](https://github.com/cesanta/mongoose-os/tree/master/fw/examples/c_mqtt)
example, build and flash it:

```bash
mos build --arch TARGET # esp8266 or cc3200
mos flash
```

Use `mos aws-iot-setup` command to register your device with the AWS IoT:

```bash
mos aws-iot-setup
```

That single command performs the certificate management for you, and
onboard your device on AWS IoT cloud. If your device has an
[Atmel ECC508A](http://www.atmel.com/devices/ATECC508A.aspx) secure element
attached, then Mongoose Firmware will use ECC508A chip for TLS handshake
and keep your credentials secure.
