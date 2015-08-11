# Smart.js platform

Smart.js is a generic, hardware independent, full-stack
Internet of Things software platform.
Smart.js solves problems of reliability, scalability, security
and remote management which are common to all verticals, being it industrial
automation, healthcare, automotive, home automation, or other.

Take a look at 2 minute video that shows Smart.js in action:

[![Smart.js in action](https://docs.cesanta.com/images/Smart.js.clip.png)](https://www.youtube.com/watch?v=6DYfGsqQzCg)

# Quick start guide

1. Download [Flashnchips](https://github.com/cesanta/smart.js/releases)
   firmware burning tool
2. Connect the board to your computer via the USB or serial interface
3. Start Flashnchips, press "Detect Devices" button
4. Press "Load Firmware" button. That will start a burning process
   Flashnchips generates random device ID and password (PSK) for the cloud
   registration.
5. When burning is complete, Smart.js automatically connects a console
   to the device, prints generated ID and password, boot messages,
   and shows an interactive JavaScript prompt. Two numbers shown by prompt
   are available free memory, and memory taken by Smart.js
   ![](https://docs.cesanta.com/images/smartjs_quick_start_guide/fc2.png)
6. Type some JavaScript expression to the console and press enter.
   Smart.js evaluates the expression and prints evaluation result:
   ![](https://docs.cesanta.com/images/smartjs_quick_start_guide/fc3.png)
7. Configure Wifi:
   enter `Wifi.setup('WifiNetworkName', 'WifiPassword')` to the console.
   When network is configured, device starts to send random numbers
   to `cloud.cesanta.com` every second, simulating real sensor data.
   `cloud.cesanta.com` however will reject that data, because it doesn't
   accept any data from unregistered devices
8. Register the device on the cloud:
   login to [https://cloud.cesanta.com](https://cloud.cesanta.com)
9. Click on "Add Device" tab, copy/paste device ID and password and press
   "Add Device" button
10. Swith to the "Dashboard" tab, and see real-time graph updated:
  ![](https://docs.cesanta.com/images/smartjs_quick_start_guide/dash1.png)

# Architecture

Technically, Smart.js has a device part and a cloud part.

![](https://docs.cesanta.com/images/smartjs_diagram.png)

Smart.js firmware on a device side:

- Allows scripting for fast and safe development & firmware update.
  We do that by developing world's smallest
  [JavaScript engine](https://github.com/cesanta/v7/).
- Provides hardware and networking API that guarantees reliability,
  scalability and security out-of-the-box.
- Devices with our software can be managed remotely and update software
  remotely, in a fully automatic or semi-automatic way.

# Supported hardware

- Espressif ESP8266 (since ALPHA1)
- Many more will be added soon!

# JavaScript API reference

## String

Smart.js has several non-standard extensions for `String.prototype` in
order to give a compact and fast API to access raw data obtained from
File, Socket, and hardware input/output such as I2C.
Smart.jsIO API functions return
string data as a result of read operations, and that string data is a
raw byte array. ECMA6 provides `ArrayBuffer` and `DataView` API for dealing
with raw bytes, because strings in JavaScript are Unicode. That standard
API is too bloated for the embedded use, and does not allow to use handy
String API (e.g. `.match()`) against data.

Smart.js internally stores strings as byte arrays. All strings created by the
String API are UTF8 encoded. Strings that are the result of
input/output API calls might not be a valid UTF8 strings, but nevertheless
they are represented as strings, and the following API allows to access
underlying byte sequence:

- `String.prototype.at(position) -> number or NaN` return byte at index
  `position`. Byte value is in 0,255 range. If `position` is out of bounds
  (either negative or larger then the byte array length), NaN is returned.
  Example: `"ы".at(0)` returns 0xd1.
- `String.prototype.blen -> number` return string length in bytes.
  Example: `"ы".blen` returns 2. Note that `"ы".length` is 1, since that
  string consists of a single Unicode character (2-byte).

## File

File API is a wrapper around standard C calls `fopen()`, `fclose()`,
`fread()`, `fwrite()`, `rename()`, `remove()`.

- `File.open(file_name [, mode]) -> file_object or null`
  Open a file `path`. For
  list of valid `mode` values, see `fopen()` documentation. If `mode` is
  not specified, mode `rb` is used, i.e. file is opened in read-only mode.
  Return an opened file object, or null on error. Example:
  `var f = File.open('/etc/passwd'); f.close();`
- `file_obj.close() -> undefined`
  Close opened file object.
  NOTE: it is user's responsibility to close all opened file streams. V7
  does not do that automatically.
- `file_obj.read() -> string`
  Read portion of data from
  an opened file stream. Return string with data, or empty string on EOF
  or error.
- `file_obj.readAll() -> string`
  Same as `read()`, but keeps reading data until EOF.
- `file_obj.write(str) -> num_bytes_written`
  Write string `str` to the opened file object. Return number of bytes written.
- `File.rename(old_name, new_name) -> errno`
  Rename file `old_name` to
  `new_name`. Return 0 on success, or `errno` value on error.
- `File.remove(file_name) -> errno`
  Delete file `file_name`.
  Return 0 on success, or `errno` value on error.
- `File.list(dir_name) -> array_of_names`
  Return a list of files in a given directory, or `undefined` on error.

Note: some file systems, e.g. SPIFFS on ESP8266 platform, are flat. They
do not support directory structure. Instead, all files reside in the
top-level directory.

## HTTP

Http API provides a simple HTTP client:

- `Http.get(url, cb)` performs a HTTP GET request at the given url, and invokes the provided callback `cb` with data and error.
- `Http.post(url, d, cb)` performs a HTTP POST request at the given url, passing `d` as body (stringified) and invokes the provided callback `cb` with data and error.

Example:

    Http.get("http://jsonip.com", function(data, error) {
      if (error) {
        print("error ", error);
      } else {
        print("my ip is ", JSON.parse(data).ip);
      }
    });

## GPIO

- `GPIO.setmode(pin, mode, pull) -> true or false` set pin mode. 'mode' is number,  0 enables both input and output, 1 enables input only, 2 enabled output only, 3 enables interruptions on pin, see `GPUI.setisr`. `pull` is a number, 0 leaves pin floating, 1 connects pin to internal pullup resistor, 2 connects pin to internal pulldown resistor.
- `GPIO.read(pin_num) -> 0 or 1` return GPIO pin level
- `GPIO.write(pin_num, true_or_false) -> true of false` set a given pin
  to `true` or `false`, return false if paramaters are incorrect
- `GPIO.setisr(pin, isr_type, func) -> true or false` assign interrruption handler for
  pin. `isr_type` is a number, 0 disables interrupts, 1 enables interupts on positive edge, 2 - on negative edge, 3 - on any edge, 4 - on low level, 5 - on high level, 6 - button mode. `func` is callback to be called on interrupt, its prototype is `function myisr(pin, level)`.

  See [button helper](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/fs/gpio.js) for `button mode` usage example.

## SPI

Constructor:
- `var spi = new SPI()`.

API:
- `spi.tran(dataToSend, [bytesToRead, command, address]) -> number` - send and receive data within one transaction. `dataToSend` - 32bit number to send to SPI. `bytesToRead` - number of bytes to read from SPI (1-4). If device requires explicit command and address, they might be provided via `command` and `address` parameters.
- `spi.txn(commandLenBits, command, addrLenBits, address, dataToSendLenBits, dataToWrite, dataToReadLenBits, dummyBits) -> number` - send and receive data within one transaction. The same as `spi.tran`, but allows to use arbitrary (1-32 bits) lengths. This function should be used if device requires, for example, 9bit data, 7bit address, 3bit command etc.

There is a detailed description in [the source file](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/user/spi.h).

See [barometer driver](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/fs/MPL115A1.js) for usage example.

## I2C

Constants:
- Communication modes: `I2C.READ`, `I2C.WRITE`
- Acknowledgement types: `I2C.ACK`, `I2C.NAK`, `I2C.NONE`

Constructor:
- `var i2c = new I2C(sda_gpio, scl_gpio)`.

Low-level API:
- `i2c.start(addr, mode) -> ackType` - claims the bus, puts the slave address on it and reads ack/nak. addr is the 7-bit address (no r/w bit), mode is either I2C.READ or I2C.WRITE.
- `i2c.stop()` - put stop condition on the bus and release it.
- `i2c.send(data) -> ackType` - send data to the bus. If `data` is a number between 0 and 255, a single byte is sent. If `data` is a string, all bytes from the string are sent. Return value is the acknowledgement sent by the receiver. When a multi-byte sequence (string) is sent, all bytes must be positively acknowledged by the receiver, except for the last one. Acknowledgement for the last byte becomes the return value. If one of the bytes in the middle was not acknowledged, `I2C.ERR` is returned.
- `i2c.readByte([ackType]) -> number` - read a byte from the slave and `ACK`/`NAK` as instructed. The argument is optional and defaults to `ACK`. It is possible to specify `NONE`, in which case the acknoewledgment bit is not transmitted, and the call must be followed up by `sendAck`.
- `i2c.readString(n, [lastAckType]) -> string` - read a sequence of `n` bytes. Ann bytes except the last are `ACK`-ed, `lastAckType` specifies what to do with the last one and works like `ackType` does for `readByte`.
- `i2c.sendAck(ackType)` - send an acknowledgement. This method must be used after one of the `read` methods with `NONE` ack type.

High-level API:
- `i2c.read(addr, nbytes) -> string|I2C.ERR` - issues a read request to the
device with address `addr`, reading `nbytes` bytes. Acknowledges all incoming
bytes except the last one.
- `i2c.write(addr, data) -> ackType` - issues a write request to the device with
address `addr`. `data` is passed as is to `.send` method.
- `i2c.do(addr, req, ...) -> array` - issues multiple requests to the same
device, generating repeated start conditions between requests. Each request is
an array with 2 or 3 elements:

  0. `I2C.READ` or `I2C.WRITE` for read or write respectively.
  1. number of bytes for read request or data to send for write request
  2. optional, different meaning for different types of requests:
    - read: `ackType` for the last read byte (defaults to `I2C.NAK`)
    - write: `ackType` to expect from the device after last sent byte (defaults
      to `I2C.ACK`)

  Return value is an array that contains the one element for each request on
  success (string data for reads, `ackType` for writes), or possibly less than
  that on error, in which case last element will be `I2C.ERR`. Errors include:
    - Address wasn't ACK'ed (no such device on the bus).
    - Device sent NACK before all the bytes were written.
    - `ackType` for the last byte written doesn't match what was expected.

There is a detailed description of this API in [the source file](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/user/v7_i2c_js.c).

See [temperature sensor driver](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/fs/MCP9808.js) and [EEPROM driver](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/fs/MC24FC.js) for usage example.

## Watchdog timer (ESP8266 specific)

ESP8266 includes a watchdog timer, a mechanism designed to deal with software
lockups. This timer is periodically reset by the OS, or can be explicitly
"fed" by user code. Failure to do so will cause the device to be reset.
The current default watchdog timer period is about 1 minute.

- `OS.wdt_feed()` - delay restart of the device for 1 minute. This function
  has to be called inside long loops, or other long operations to
  prevent device reset.

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

## Cloud

This interface provides an easy way to send data to the
[Cesanta cloud](https://cloud.cesanta.com/). On a cloud side, it is easy to build interactive
real-time dashboards.

- `Cloud.store(name, value [, options]) -> undefined` - store metric `name`
with value `value` in a cloud storage. Optional `options` object can be
used to specify metrics labels and success callback function. Example:
`Cloud.store('temperature', 36.6)`. The following prerequisites has
to be met:
  - Wifi needs to be configured
  - Global configuration object `conf` needs to have device ID and
  password set, `conf.dev.id` and `conf.dev.psk`
  - Device with those ID and PSK needs to be registered in a cloud - see
    video at the top of this document

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
- `Debug.print(...)` - print information to current debug output (set by `Debug.mode`)

# Building Smart.js firmware

For those who want to build Smart.js firmware themselves, here is an
instruction on how to do that:

1. Make sure you have [Docker](https://www.docker.com/) installed and running
2. Execute the following:

```
$ git clone https://github.com/cesanta/smart.js.git
$ cd smart.js/platforms/esp8266
$ sh make.sh
```

The firmware gets built in the `firmware/` folder. Copy it to the
`Fish'n'chips`'s `firmware/esp8266/` directory, and it'll be ready to flash!

Notes for Windows users:
- Operations above should be performed from `boot2docker` console, not from `cmd.exe`
- Only `C:\Users` is mapped into docker's VM by default, so put cloned repo to `Users` subfolder

Notes for Mac users:
- Only `/Users` is mapped into docker's VM by default, so put cloned repo to `Users` subfolder

# Extending Smart.js firmware

It is trivial to add more API functions to the Smart.js firmware.
Smart.js is built on top of
[V7 JavaScript engine](https://github.com/cesanta/v7/) which makes it easy
to export C/C++ functions to JavaScript:

- [V7 reference on exporting C/C++ functions to JS](https://docs.cesanta.com/v7/#_call_c_c_function_from_javascript)
- See `init_v7()` function at [v7_esp.c](https://github.com/cesanta/smart.js/blob/master/platforms/esp8266/user/v7_esp.c) for an example of how specific C/C++ API is exported to ESP8266 firmware. To extend it, just edit `init_v7()` function and rebuild the firmare by running `sh make.sh` in `smartjs/platforms/esp8266` directory.

# Contributions

People who have agreed to the
[Cesanta CLA](https://docs.cesanta.com/contributors_la.html)
can make contributions. Note that the CLA isn't a copyright
_assigment_ but rather a copyright _license_.
You retain the copyright on your contributions.

# Licensing

Smart.js is released under the commercial and
[GNU GPL v.2](http://www.gnu.org/licenses/old-licenses/gpl-2.0.html) open
source licenses. The GPLv2 open source License does not generally permit
incorporating this software into non-open source programs.
For those customers who do not wish to comply with the GPLv2 open
source license requirements,
[Cesanta](https://www.cesanta.com) offers a full,
royalty-free commercial license and professional support
without any of the GPL restrictions.
