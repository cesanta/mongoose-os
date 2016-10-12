Mongoose Firmware networking skeleton
=====================================

# Overview

Mongoose Firmware is a firmware skeleton that provides a set of
essential IoT-specific functionalities for the firmware developers:

- Rich networking capabilities provided by
  [Mongoose Embedded Networking Library](https://github.com/cesanta/mongoose),
  trusted by organisations like NASA, Qualcomm, Intel, Samsung, etc
- Filesystem with POSIX API (SPIFFS)
- Flexible [configuration management](https://docs.cesanta.com/mongoose-iot/master/#/firmware/configuration.md/) with 3 layers: firmware
  defaults, factory defaults, and end-customer overrides
- Secure and reliable over-the-air (OTA) updates: update only parts that
  changed, and rollback on failures

# Supported Hardware architectures

- TI CC3200
- ESP8266
- In progress:
   - STM32
   - nRF52

# How to build

```
$ make PLATFORM=cc3200     # Build for CC3200 platform
$ make PLATFORM=esp8266    # Build for ESP8266 platform
```

Firmware files will be stored in the firmware directory.

# How to flash

Download [Mongoose Flashing Tool](https://github.com/cesanta/mft/releases).
Then run the following commands on a console:

```
$ MFT --platform=cc3200  --port=/dev/ttyUSB1 --flash firmware/*-last.zip --console
$ MFT --platform=esp8266 --port=/dev/ttyUSB0 --flash firmware/*-last.zip --console
```
The commands are shown for CC3200 and ESP8266 respectively.


# How to test Over-the-air (OTA) update

- Build a firmware and flash it on to the device.
- Device will start an access point `Mongoose_XXXXXX`. Connect to it,
  use `Mongoose` as a password.
- Make some modifications to the firmware, for example add this code to
`mg_app_init()`:

```c
    LOG(LL_INFO, ("Hey!"));
```
- Build a new firmware
- Update to a new firmware over-the-air:
```
$ curl -v -F file=@firmware/mongoose-iot-cc3200-last.zip http://192.168.4.1/update
```

On a serial console, you might see log messages like this:

```
main_task            Mongoose IoT Firmware 20160620-154812/cc3200@87511eed+
main_task            RAM: 48620 total, 34456 free
mg_wifi_on_change_callback WiFi: ready, IP 192.168.4.1
start_nwp            NWP v2.6.0.5 started, host driver v1.0.1.6
main_task            Boot cfg 1: 0xfffffffffffffffd, 0x3, mongoose-iot.bin.0 @ 0x20000000, spiffs.img.1
main_task            Applying update
file_copy            Copying test.txt
init_device          MAC: F4B85E49A7B3
mg_wifi_on_change_callback WiFi: ready, IP 192.168.4.1
mg_wifi_setup_ap     AP Mongoose_49A7B3 configured
init_web_server      HTTP server started on [80]
main_task            Sys init done, RAM: 14944 free
mg_app_init          Hey!
main_task            App init done
commit_update        Committed
```
