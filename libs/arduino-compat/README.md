# Arduino compatibility for Mongoose OS

This library provides a common Arduino compatibility layer, so that one could
pick an existing Arduino program, throw it into the Mongoose OS application
sources, and ideally, it "just works".

Currently, the following public headers are provided:

- `Arduino.h`
- `Print.h`
- `WString.h`
- `stdlib_noniso.h`

There are more specific Arduino-compatibility libraries available: for
[onewire](https://github.com/cesanta/mongoose-os/libs/arduino-onewire),
[SPI](https://github.com/cesanta/mongoose-os/libs/arduino-spi), etc.
