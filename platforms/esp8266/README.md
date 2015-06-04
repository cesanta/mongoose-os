Overview
========

This document describes Smartjs firmware for ESP8266 chip.

Smartjs firmware allows to manipulate hardware over GPIO/I2C/SPI interfaces,
set up WiFi, and communicate over the network - all using simple JavaScript
commands described below.

ESP8266 board can be accessed through a serial port, or directly via browser if WiFi is configured.

Smartjs firmware can be easily extended by adding custom commands - refer to
http://cesanta.com/docs/v7/ to learn how to export custom functions, and look
at the examples at `user/v7_cmd.c`.

## Build the firmware

Smartjs release provides pre-built firmare, so you might just download
it from the
[Smartjs release page](https://github.com/cesanta/smart.js/releases) and
skip to the next step. For those who want to build/customize the firmware,
make sure you have docker installed and running. Then,

    sh make.sh

This will produce three binary images under the firmware/ subdirectory.

## Flash the firmware
To flash the firmware to the ESP8266 board,

1. Get `esptool` utility, available at https://github.com/themadinventor/esptool)
2.  Make sure ESP8266 is connected to the serial port
3. `esptool --baud 115200 --port /dev/YOUR_ESP_SERIAL_DEVICE write_flash 0x00000 firmware/0x00000.bin 0x20000 firmware/0x20000.bin 0x30000 firmware/0x30000.bin`


## Communication over the serial port

On Mac or UNIX/Linux system, `cu` or `picocom` tools can be used:

    picocom -b 115200 --omap crcrlf /dev/YOUR_ESP_SERIAL_DEVICE
    cu -s 115200 -l /dev/YOUR_ESP_SERIAL_DEVICE

For example `cu` tool on Linux:
    cu -s 115200 -l /dev/ttyUSB0

## Wifi module

By default, Wifi module enables access point mode, and acts as a
DHCP server for it. To connect to local Wifi network:

    Wifi.setup("yourssid", "youpassword")

To check current WiFi status:

    Wifi.status()

To see your ip address:

    Wifi.ip()

NOTE: `Wifi.ip(1)` shows the IP of the access point interface.

## Web UI

Point your browser to: http://<YOUR_ESP8266_IP_ADDRESS>/

## Example: blink sketch

Connect LED to GPIO pin number 2. Then type this at the prompt and press enter:

    led = true; function blink() { GPIO.out(2, led); led = !led; setTimeout(blink, 1000); }; blink()
