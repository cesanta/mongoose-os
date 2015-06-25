# Overview

This document describes Smartjs firmware for ESP8266 chip.

Smartjs firmware allows to manipulate hardware over GPIO/I2C/SPI interfaces,
set up WiFi, and communicate over the network - all using simple JavaScript
commands described below.

ESP8266 board can be accessed through a serial port, or directly via browser if WiFi is configured.

Smartjs firmware can be easily extended by adding custom commands - refer to
http://cesanta.com/docs/v7/ to learn how to export custom functions, and look
at the examples at `user/v7_cmd.c`.

# Build the firmware

Smartjs release provides pre-built firmare, so you might just download
it from the
[Smartjs release page](https://github.com/cesanta/smart.js/releases) and
skip to the next step. For those who want to build/customize the firmware,
make sure you have docker installed and running. Then,

    sh make.sh

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

# JavaScript API reference

## GPIO

- `GPIO.read(pin_num) -> true or false` return GPIO pin status
- `GPIO.write(pin_num, true_or_false) -> undefined` set a given pin
  to `true` or `false`

## I2C

- `i2c.init(sda_pin_no,scl_pin_no)` - initialize i2c connection
  through given pin numbers, return I2C object for further communication
- `I2C.start()` - send "start" signal to a slave
- `I2C.stop()` - send "stop" signal to a slave
- `I2C.sendAck()` - send "ack" to a slave
- `I2C.sendNack()` - send "nack" to a slave
- `I2C.getAck()` - read ack/nack from a slave, return 0 for "ack" and "1" for nack
- `I2C.readByte()` - read one byte from a slave
- `I2C.sendByte(byte)` - send given byte to a slave, return 0 if slave acked byte or 1 otherwise
- `I2C.sendWord(word)` - send two bytes in network order, return 0 if slave acked both bytes or 1 otherwise
- `I2C.sendString(string)` - send string to a slave, return 0 if slave acked all bytes or 1 otherwise
- `I2C.readString(len)` - read `len` bytes to string, send ack for all bytes except last, which is nacked.
- `I2C.close()` - close i2c connection and close connection

See [temperature sensor driver](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/fs/MCP9808.js) and [eeprom driver](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/fs/MC24FC.js) for usage example.

## HTTP


## Wifi

By default, Wifi module enables access point mode, and acts as a
DHCP server for it.

- `Wifi.setup("yourssid", "youpassword") -> true or false` - connect
  to the local Wifi network
- `Wifi.status() -> status_string` - check current Wifi status
- `Wifi.ip() -> ip_address_string` - get assigned IP address.
  `Wifi.ip(1)` returns IP address of the access point interface.
- `Wifi.mode(mode) -> true or false` - set Wifi mode. `mode` is a number,
  1 is station, 2 is soft-AP, 3 is station + soft-AP

## Built-in functions

- `usleep(num_microseconds) -> undefined` - sleep for `num_microseconds`
- `dsleep(num_microseconds [, dsleep_option]) -> undefined` - deep sleep for
  `num_microseconds`. If `dsleep_option` is specified, ESP's
  `system_deep_sleep_set_option(dsleep_option)` is called prior to going to
  sleep. The most useful seems to be 4 (keep RF off on wake up,
  reduces power consumption).
- `setTimeout(callback, num_milliseconds) -> undefined` - schedule
  function call after `num_milliseconds`.
- `print(arg1, ...) -> undefined` - stringify and print
  arguments to the command prompt
- `GC.stat() -> stats_object` - return current memory usage
- `Debug.mode(mode) -> status_number` - set redirection for system
  and custom (stderr) error logging: 0 = /dev/null, 1 = uart0, 2 = uart1

Also, Smart.js has support for a flat filesystem. File API is described
at [V7 API Reference](http://cesanta.com/docs/v7/#_builtin_api).

# Web UI

Smart.js has a built-in web server which shows a web-based UI.
Point your browser to: http://YOUR_ESP8266_IP_ADDRESS/ to access it.

# Example: blink sketch

Connect LED to GPIO pin number 2. Then type this at the prompt and press enter:

    led = true; function blink() { GPIO.out(2, led); led = !led; setTimeout(blink, 1000); }; blink()


# Advanced

## Build options

- NO_PROMPT: disables serial javascript prompt
- NO_EXEC_INITJS: disables running initjs at boot
- NO_HTTP_EVAL: disables http eval server
- V7_ESP_GDB_SERVER: enables GDB server
- ESP_ENABLE_WATCHDOG: enables watchdog (the watchdog is not fed nor we provide yet a way to feed it from JS, the ESP will be reset if you execute time consuming operations!)

## GDB

This build also includes an optional GDB server, enabled with the -DV7_GDB compiler flag.
When enabled, each illegal instruction or memory access will trap into the GDB server.

This can be very useful both to debug your custom C code, and to provide us with stack traces
for debugging issues in the JS VM and other parts of the SmartJS SDK.

The user can attach with a gdb build supporting the lx106 framework. We offer such a binary
in our cesanta/esp8266-build-oss docker image. Please make sure you use at least version 1.1.0
(if 1.1.0 is not yet the default, you can fetch it as cesanta/esp8266-build-oss:1.1.0).

The docker image cannot access the device serial port so you should create a proxy. You can use
either socat or our http://github.com/cesanta/gopro tool.

    ./gopro -s :1234 -d /dev/tty.SLAB_USBtoUART -b 115200

Then you invoke the gdb inside the docker image:

    xt-gdb /cesanta/smartjs/platforms/esp8266/build/app.out
    remote target <yourhost>:1234

The GDB server is incomplete but should be good enough to print a stack trace
and inspect the state of your application. Breakpoints and resuming are not yet supported.
