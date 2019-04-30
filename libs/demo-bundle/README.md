# A collection of libraries for demoing Mongoose OS


## Overview

This library is intended to be used for apps that demonstrate
Mongoose OS capabilities. It is essentially a collection of libraries that
implement a wide set of functionalities - from hardware peripherals API
to cloud integrations like AWS IoT, Google IoT Core, etc.

Also, this library introduces `pins.led` and `pins.button`
configuration settings:

```yaml
config_schema:
  - ["pins", "o", {title: "Pins layout"}]
  - ["pins.led", "i", -1, {title: "LED GPIO pin"}]
  - ["pins.button", "i", -1, {title: "Button GPIO pin"}]
```

For different hardware boards, these are set to different values. This
allows the same code to work on different boards without modifications.