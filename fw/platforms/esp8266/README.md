# Overview

This document describes Mongoose IoT firmware for ESP8266 chip.

Mongoose IoT firmware allows to manipulate hardware over GPIO/I2C/SPI interfaces,
set up WiFi, and communicate over the network - all using simple JavaScript
commands described below.

ESP8266 board can be accessed through a serial port, or directly via browser if WiFi is configured.

Mongoose IoT firmware can be easily extended by adding custom commands - refer to
https://docs.cesanta.com/v7/ to learn how to export custom functions, and look
at the examples at `user/v7_cmd.c`.

# Build the firmware

Mongoose IoT release provides pre-built firmare, so you might just download
it from the
[Mongoose IoT release page](https://github.com/cesanta/mongoose-iot/releases) or
[developer builds](https://backend.cesanta.com/devel/mongoose-iot-esp8266-last.zip) and
skip to the next step. For those who want to build/customize the firmware,
make sure you have docker installed and running. Then,

    make

This will produce three binary images under the firmware/ subdirectory.

# Flash the firmware
To flash the firmware to the ESP8266 board,

1. Get `esptool` utility, available at https://github.com/themadinventor/esptool)
2.  Make sure ESP8266 is connected to the serial port
3. `esptool --baud 115200 --port /dev/YOUR_ESP_SERIAL_DEVICE write_flash 0x00000 firmware/0x00000.bin 0x20000 firmware/0x20000.bin 0x30000 firmware/0x30000.bin`


# Communication over the serial port

On Mac or UNIX/Linux system, `cu` or `picocom` tools can be used:

    picocom -b 115200 --omap crcrlf /dev/YOUR_ESP_SERIAL_DEVICE
    cu -s 115200 -l /dev/YOUR_ESP_SERIAL_DEVICE

For example `cu` tool on Linux:
    cu -s 115200 -l /dev/ttyUSB0

# Example: blink sketch

Connect LED to GPIO pin number 2. Then type this at the prompt and press enter:

    led = true; function blink() { GPIO.out(2, led); led = !led; setTimeout(blink, 1000); }; blink()


# Advanced

## Build options

- NO_PROMPT: disables serial javascript prompt
- NO_EXEC_INITJS: disables running initjs at boot
- NO_HTTP_EVAL: disables http eval server
- ESP_GDB_SERVER: enables GDB server
- ESP_COREDUMP: enables printing of coredump on crash
- ESP_COREDUMP_NOAUTO: disables automatic coredumping on crash
- ESP_ENABLE_WATCHDOG: enables watchdog (the watchdog is not fed nor we provide yet a way to feed it from JS, the ESP will be reset if you execute time consuming operations!)

## ADC

ESP8266 has a 10-bit ADC with input voltage range of 0 - 1V. To measure higher values you will need an external voltage divider.

## GDB

This build also includes an optional GDB server, enabled with the -DESP_GDB_SERVER compiler flag.
When enabled, each illegal instruction or memory access will trap into the GDB server.

This can be very useful both to debug your custom C code, and to provide us with stack traces
for debugging issues in the JS VM and other parts of the Mongoose IoT SDK.

The user can attach with a gdb build supporting the lx106 framework. We offer such a binary
in our cesanta/esp8266-build-oss docker image. Please make sure you use at least version 1.3.0
(if 1.3.0 is not yet the default, you can fetch it as cesanta/esp8266-build-oss:1.3.0-cesanta).

The docker image cannot access the device serial port so you should create a proxy. You can use
either socat or our http://github.com/cesanta/gopro tool.

    ./gopro -s :1234 -d /dev/tty.SLAB_USBtoUART -b 115200

Then you invoke the gdb inside the docker image:

    xt-gdb /cesanta/fw/platforms/esp8266/build/fw.out
    remote target <yourhost>:1234

The GDB server is incomplete but should be good enough to print a stack trace
and inspect the state of your application. Breakpoints and resuming are not yet supported.

## Core dumps

This build can also dump memory and machine state in via the uart in a format that can be later
used to perform post mortem debugging. The feature is enabled with -DESP_COREDUMP.

By default core dumps will be output on the debug uart (uart 1), but at each crash the user
can request a dump on uart 0 (main serial console).
The core dump is textual snippet of a few hundreds KB. Your terminal emulation software
might help you save it into a file, or you might just copy&paste it.

    tools/debug_coredump.sh mylogfile.log

This option is very useful when:

1. You don't want to debug it straight away, but you want to save the state for later inspection.
2. The device that crashed is in a remote location and it's hard to setup a portforwarding and serial->tcp paraphernalia in order to connect it to gdb.
3. While live GDB cannot resume or do other cool things, it doesn't make a big difference to do live or post mortem debugging. Using tools/debug_coredump.sh doesn't require you to type complicated commands and know your IP address.
4. Your device is using uart0 to do some real work and your only troubleshooting channel is uart1 which is unidirectional, and hence cannot be used for the interactive debugger.
