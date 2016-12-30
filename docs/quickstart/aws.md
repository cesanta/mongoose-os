---
title: AWS IoT integration
---

Mongoose Firmware is integrated natively with AWS IoT cloud.

AWS IoT uses MQTT for communication, so navigate to the
[c_mqtt](https://github.com/cesanta/mongoose-iot/tree/master/fw/examples/c_mqtt)
example, build and flash it:

```bash
mgos build --arch TARGET # esp8266 or cc3200
mgos flash
```

Use `mgos aws-iot-setup` command to register your device with the AWS IoT:

```bash
mgos aws-iot-setup
```

That single command performs the certificate management for you, and
onboard your device on AWS IoT cloud. If your device has an
[Atmel ECC508A](http://www.atmel.com/devices/ATECC508A.aspx) secure element
attached, then Mongoose Firmware will use ECC508A chip for TLS handshake
and keep your credentials secure.

