Mongoose Firmware networking skeleton
=====================================


See [quick start guide](https://mongoose-iot.com/docs/#/quickstart/)
on how to build and flash the firmware.

# How to test Over-the-air (OTA) update

- Build a firmware and flash it on to the device.
- Device will start an access point `Mongoose_XXXXXX`. Connect to it,
  use `Mongoose` as a password.
- Make some modifications to the firmware, for example add this code to
`miot_app_init()`:

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
miot_wifi_setup_ap   AP Mongoose_49A7B3 configured
init_web_server      HTTP server started on [80]
main_task            Sys init done, RAM: 14944 free
miot_app_init        Hey!
main_task            App init done
commit_update        Committed
```
