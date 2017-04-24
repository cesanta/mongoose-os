# mJS UART API usage example

## Overview

This example demonstrates how to use [mJS UART API](https://github.com/cesanta/mongoose-os/blob/master/fw/mjs_api/api_uart.js).
For C API usage example see [here](https://github.com/cesanta/mongoose-os/tree/master/fw/examples/c_uart).

## Running

See [quick start guide](https://mongoose-os.com/docs/#/quickstart/)
on how to build and flash the firmware. Below is an example session:

```bash
mos flash ARCH    # Flash the firmware. ARCH: esp8266, esp32, cc3200, stm32
mos put init.js   # Put init.js on the device
mos console       # Attach a serial console and see device logs
```

See https://github.com/cesanta/mjs for the MJS engine information.
