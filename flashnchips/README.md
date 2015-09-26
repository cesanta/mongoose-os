# Flash’N’Chips

Flash’N’Chips is the Smart.js flashing tool. It is designed to be simple, but
provide advanced options via command-line flags.

## Firmware format

Flash’N’Chips expects the firmware to be located next to the executable in a
directory called 'firmware'. This directory must contain a subdirectory named
after the platform (e.g. 'esp8266'), that should contain one subdirectory for
each firmware. For example:

```
flashnchips.exe
firmware/
└── esp8266
    └── Smart.js
        ├── 0x00000.bin
        ├── 0x1d000.bin
        └── 0x6d000.bin
```

### ESP8266

For ESP8266 firmware consists of one or more files with named `0x*.bin`. File
name is interpreted as a hexadecimal number which is used as an offset on flash
where the content of the file needs to be written.

See also [ESP8266-specific](../platforms/esp8266/flashing.md) notes on wiring.

## Build

Flash’N’Chips requires:

- Qt 5
- libftdi

Build with:

```
$ qmake
$ make
```
