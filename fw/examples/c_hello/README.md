This is a C-only Mongoose IoT application.

Compile with `PLATFORM` set:

```
make PLATFORM=esp8266 && \
  FNC --platform=esp8266 --port=/dev/ttyUSB0 --flash-baud-rate=1500000 --flash firmware/*-last.zip --console
```

or


```
make PLATFORM=cc3200 && \
  FNC --platform=cc3200 --port=/dev/ttyUSB1 --flash firmware/*-last.zip --console
```

FNC is the [Flash 'n Chips](https://github.com/cesanta/fnc) flashing tool, it is optional (but convenient, try it).
