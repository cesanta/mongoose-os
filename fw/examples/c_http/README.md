# This is a C-only Mongoose OS application.


This example shows how to control a device remotely using HTTP protocol.

Assuming you have downloaded the
[mos utility](https://mongoose-os.com/software.html).
Please follow these steps:

```Bash
git clone https://github.com/cesanta/mongoose-os
cd mongoose-os/fw/examples/c_http
export MOS_PORT=/dev/ttyUSB0  # or, whatever is correct on your system
mos build --arch ARCHITECTURE # cc3200 or esp8266
mos flash
mos config-set wifi.ap.enable=false wifi.sta.enable=true \
	wifi.sta.ssid=YOUR_WIFI_NETWORK wifi.sta.pass=YOUR_WIFI_PASSWORD
mos console
```

The `mos console` command attaches to the device's UART and prints
log messages. Notice the IP address assigned to the module.

In a separate terminal, toggle a GPIO pin. For example, on NodeMCU pin
number 2 is wired to the built-in blue LED. Turn it on:

```
curl -d '{pin:2 state: 0}' http://IP_ADDRESS_/ctl
```
