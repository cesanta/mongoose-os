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

- `GPIO.setmode(pin, mode, pull) -> true or false` set pin mode. 'mode' is number,  0 enables both input and output, 1 enables input only, 2 enabled output only, 3 enables interruptions on pin, see `GPUI.setisr`. `pull` is a number, 0 leaves pin floating, 1 connects pin to internal pullup resistor, 2 connects pin to internal pulldown resistor.
- `GPIO.read(pin_num) -> 0 or 1` return GPIO pin level
- `GPIO.write(pin_num, true_or_false) -> true of false` set a given pin
  to `true` or `false`, return false if paramaters are incorrect
- `GPIO.setisr(pin, isr_type, func) -> true or false` assign interrruption handler for
  pin. `isr_type` is a number, 0 disables interrupts, 1 enables interupts on positive edge, 2 - on negative edge, 3 - on any edge, 4 - on low level, 5 - on high level. `func` is callback to be called on interrupt, its prototype is `function myisr(pin, level)`.

## I2C

Constants:
- Communication modes: `I2C.READ`, `I2C.WRITE`
- Acknowledgement types: `I2C.ACK`, `I2C.NAK`, `I2C.NONE`

Methods:
- var i2c = new I2C(sda_gpio, scl_gpio) - constructor.
- `i2c.start(addr, mode) -> ackType` - claims the bus, puts the slave address on it and reads ack/nak. addr is the 7-bit address (no r/w bit), mode is either I2C.READ or I2C.WRITE.
- `i2c.stop()` - put stop condition on the bus and release it.
- `i2c.send(data) -> ackType` - send data to the bus. If `data` is a number between 0 and 255, a single byte is sent. If `data` is a string, all bytes from the string are sent. Return value is the acknowledgement sent by the receiver. When a multi-byte sequence (string) is sent, all bytes must be positively acknowledged by the receiver, except for the last one. Acknowledgement for the last byte becomes the return value. If one of the bytes in the middle was not acknowledged, `I2C.ERR` is returned.
- `i2c.readByte([ackType]) -> number` - read a byte from the slave and `ACK`/`NAK` as instructed. The argument is optional and defaults to `ACK`. It is possible to specify `NONE`, in which case the acknoewledgment bit is not transmitted, and the call must be followed up by `sendAck`.
- `i2c.readString(n, [lastAckType]) -> string` - read a sequence of `n` bytes. Ann bytes except the last are `ACK`-ed, `lastAckType` specifies what to do with the last one and works like `ackType` does for `readByte`.
- `i2c.sendAck(ackType)` - send an acknowledgement. This method must be used after one of the `read` methods with `NONE` ack type.

There is a detailed description of this API in [the source file](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/user/v7_i2c_js.c).

See [temperature sensor driver](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/fs/MCP9808.js) and [EEPROM driver](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/fs/MC24FC.js) for usage example.

## HTTP

## Watchdog timer

ESP8266 includes a watchdog timer, a mechanism designed to deal with software lockups. This timer is periodically reset by the OS, or can be explicitly "fed" by user code. Failure to do so will cause the device to be reset.
The current default watchdog timer period is about 1 minute.
- `OS.wdt_feed()` - delay restart of the device for 1 minute. This function has to be called inside long loops, or other long operations to prevent device reset.

## Wifi

By default, Wifi module enables access point mode, and acts as a
DHCP server for it.

- `Wifi.setup("yourssid", "youpassword") -> true or false` - connect
  to the local Wifi network
- `Wifi.status() -> status_string` - check current Wifi status
- `Wifi.ip() -> ip_address_string` - get assigned IP address.
  `Wifi.ip(1)` returns IP address of the access point interface.
- `Wifi.show()` - returns the current SSID
- `Wifi.changed(cb)` - invokes `cb` whenever the connection status changes:
  - 0: connected
  - 1: disconnected
  - 2: authmode changed
  - 3: got ip
  - 4: client connected to ap
  - 5: client disconnected from ap
- `Wifi.mode(mode) -> true or false` - set Wifi mode. `mode` is a number,
  1 is station, 2 is soft-AP, 3 is station + soft-AP
- `Wifi.scan(cb)` - invoke `cb` with a list of discovered networks.

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
