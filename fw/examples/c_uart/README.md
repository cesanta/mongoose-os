# C UART API usage example

## Overview

This example demonstartes how to use [C UART API](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_uart.h).
For mJS API usage example see [here](https://github.com/cesanta/mongoose-os/tree/master/fw/examples/mjs_uart).

By default, mOS is configured to use UART0 for system console: this is where
both stdout and stderr are routed. UART1 is unused, making it available for
user applications. We will assume this configuration in our example.

It will print hello message to UART1 every second and echo whatever is received
on UART1 to UART0 (console).

## Platform support

It should work on all supported architectures.

 - On ESP8266 UART1 TX is GPIO2. UART1 RX is inaccessible, so it is not
   possible to send anything.
 - On ESP32, by default, UART1 TX is GPIO14 and RX is 13.
 - On CC3200 - 7 and 8 respectively (these are pin numbers, not GPIO).

## Controlling console output

It is possible to reconfigure console output to different UART or disable
console output entirely: it is controlled by the `debug.stdout_uart` and
`debug.stderr_uart` configuration settings. Defaults for both are 0, setting
to 1 will send output to UART1 and setting to a negative value will disable
output. This is especially useful on ESP8266, where UART0 is the only
bidirectional UART (there is no UART2, despite what some diagrams may suggest
by showing RXD2 and TXD2 functions).

Early diagnostic output (before configuration file is read) is controlled by
the `MGOS_DEBUG_UART` compile-time option.

## Running

See [quick start guide](https://mongoose-os.com/docs/#/quickstart/)
on how to build and flash the firmware. Below is an example session:

```bash
mos build        # Build the firmware
mos flash        # Flash the firmware
mos console      # Attach a serial console and see device logs
```

