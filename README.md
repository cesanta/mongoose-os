# Smart.JS platform

Smart.JS is a generic, hardware independent, full-stack
Internet of Things software platform.
Smart.JS solves problems of reliability, scalability, security
and remote management which are common to all verticals, being it industrial
automation, healthcare, automotive, home automation, or other.

# Overview

Technically, Smart.JS has a device part and a cloud part.

![](http://cesanta.com/images/smartjs_diagram.png)

Smart.JS firmware on a device side:

- allows scripting for fast and safe development & firmware update.
  We do that by developing world's smallest JavaScript scripting engine.
- provides hardware and networking API that guarantees reliability,
  scalability and security out-of-the-box.
- Devices with our software can be managed remotely and update software
  remotely, in a fully automatic or semi-automatic way.

Smart.JS software on a cloud side has three main components:

- device management database that keeps information about all devices,
  e.g. unique device ID, device address, software version, et cetera.
- telemetry database with analytics. It can, for example, store information
  from remote sensors, like electricity and water meters, and able to answer
  questions like "show me a cumulative power consumption profile for plants
  A and B from 3 AM to 5 AM last night."
- remote software update manager. Schedules and drives software updates
  in a reliable way. Understands policies like "start remote update with
  device ID 1234 always. Check success in 5 minutes. If failed, roll back
  to previous version and alert. If successful, proceed with 5 more random
  devices of the same class. If successful, proceed with the rest of devices.
  Never keep more than 5% of devices in flight. If more then 0.1% updates
  fail, stop the update globally, do not roll back, and alert."


# Supported device architectures

- Texas Instruments CC3200
- Espressif ESP8266

# Smart.js firmware burning tool (stool)

For burning Smart.JS firmware to devices, we provide a `stool` utility.
Click on [releases](https://github.com/cesanta/smart.js/releases)
link to download it.

Stool utility also provides a serial console. After firmware is successfully
loaded onto the device, a serial console shows JavaScript prompt where
user can enter JavaScript commands. A prompt looks like this:

```
smartjs 12896/2676$
```

Two numbers are: the size of available system heap, and the
amount of heap is used by Smart.js. Once the command is entered, Smart.js
shows the evaluation result:

```
smartjs 12896/2676$ GPIO.read(1)
false
```

A JavaScript API reference is below.

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

# Contributions

People who have agreed to the
[Cesanta CLA](http://cesanta.com/contributors_la.html)
can make contributions. Note that the CLA isn't a copyright
_assigment_ but rather a copyright _license_.
You retain the copyright on your contributions.

# Licensing

Mongoose is released under commercial and
[GNU GPL v.2](http://www.gnu.org/licenses/old-licenses/gpl-2.0.html) open
source licenses. The GPLv2 open source License does not generally permit
incorporating this software into non-open source programs.
For those customers who do not wish to comply with the GPLv2 open
source license requirements,
[Cesanta](http://cesanta.com) offers a full,
royalty-free commercial license and professional support
without any of the GPL restrictions.
