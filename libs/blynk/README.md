# Blynk integration for Mongoose OS

This Mongoose OS library allows your device remote control via
the Blynk platform. Device side logic could be implemented in either
C/C++ or JavaScript.

Blynk is a platform with iOS and Android apps to control Arduino,
Raspberry Pi and the likes over the Internet.

See example video at:

<iframe src="https://www.youtube.com/embed/9lTIN_WRWMs"
  width="560" height="315"  frameborder="0" allowfullscreen></iframe>

## How to use this library

In your Mongoose OS app, edit `mos.yml` file and add a reference to this
library. See an [example blynk app](https://github.com/mongoose-os-apps/blynk)
that does that.

## Device configuration

This library adds `blynk` configuration section to the device:

```bash
mos config-get blynk
{
  "auth": "YOUR_BLYNK_AUTH_TOKEN",
  "enable": true,
  "server": "blynk-cloud.com:8442"
}
```

In order for your device to authenticate with Blynk cloud, either use
Web UI to change the `blynk.auth` value, or in a terminal:

```bash
mos config-set blynk.auth=YOUR_BLYNK_AUTH_TOKEN
```

