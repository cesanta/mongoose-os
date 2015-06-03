Overview
========

This demo shows how to run V7 on an ESP8266.

It contains a simple JavasScript REPL over ESP8266 serial port.
The environment exports two example C functions to the JS environment:

- setGPIO(pin, enable)
- usleep(usecs)

Read http://cesanta.com/docs/v7/ to learn how to add your own C functions.

Build
=====

This demo requires the https://github.com/pfalcon/esp-open-sdk in order to build.

In order to build, just issue:

    make

This will produce three binary images under the firmware/ subdirectory.

    make flash ESPPORT=/dev/<your_esp_serial_device>

The firmware/0x20000.bin file is a SPIFFS filesystem image containing the initial
set of files you likely want to have on your device. It's important that the memory area
allocated to the file system is preloaded with that image file or in alternative to be
completely erased.

After the first time you flash your device, you have the option to either reflash only the filesystem:

    make flash_fs ESPPORT=/dev/<your_esp_serial_device>

or flash the system while preserving the filesystem:

    make flash_no_fs ESPPORT=/dev/<your_esp_serial_device>

If you're building on OSX or otherwise don't want to install the SDK locally, we provide
a docker image with the whole toolchain and SDK preinstalled. You can use it with:

    ./make.sh

You still need esptool locally (available at https://github.com/themadinventor/esptool) in order
to flash the device with `make flash ...`. The tool is cross platform and it only requires Python.

Communication
=============

The default baud rate of the demo is 115200.

For example you can connect to the device with:

    picocom -b 115200 --omap crcrlf /dev/<your_esp_serial_device>

Wifi setup
==========

    Wifi.setup("yourssid", "youpassword")

To check your wifi status:

    Wifi.status()

To see your ip address:

    Wifi.ip()

Web UI
======

Point your browser to: http://<YOUR_ESP8266_IP_ADDRESS>/

Example
=======

Type this at the prompt:

    led = true; function blink() { GPIO.out(2, led); led = !led; setTimeout(blink, 1000); }; blink()
