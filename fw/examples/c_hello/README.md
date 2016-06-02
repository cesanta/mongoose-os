This is a C-only Mongoose IoT application.

Building requires [Docker](https://docs.docker.com/engine/installation/).

Compile with `PLATFORM` set:

```
make PLATFORM=esp8266
```
or

```
make PLATFORM=cc3200
```

Firmware files will be stored in the `firmware` directory.

To flash the firmware you can use [Flash 'n Chips](https://github.com/cesanta/fnc):

```
  FNC --platform=esp8266 --port=/dev/ttyUSB0 --flash-baud-rate=1500000 --flash firmware/*-last.zip --console
```
or
```
  FNC --platform=cc3200 --port=/dev/ttyUSB1 --flash firmware/*-last.zip --console
```
