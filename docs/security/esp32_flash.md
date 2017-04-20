---
title: ESP32 flash encryption
---

ESP32 chip comes with built-in security features, one of which is a
transparent SPI flash encryption - for details, see
[Espressif documentation](http://esp-idf.readthedocs.io/en/latest/security/flash-encryption.html).

Mongoose OS makes ESP32 flash encryption setup easy.
By default, Mongoose OS firmware is flashed in plain-text form:

```bash
mos flash esp32                                 # Flash Mongoose OS on ESP32
mos flash-read --arch esp32 0x190000 2000 -     # Dump filesystem area
```

The `flash-read` command dumps the flash memory into a file, and the output
can show that the contents is not encrypted. Therefore, sensitive
information like TLS private keys could be easily stolen from the flash.
In this case, we see a part of the device's file system, not encrypted.

In order to enable flash encryption, use `esp32-gen-key` command. It
enables flash encryption for the next flashing:

```bash
mos -X esp32-gen-key flash_encryption_key fe.key --esp32-enable-flash-encryption --dry-run=false
mos flash esp32
```

NOTE: the `esp32-gen-key` command is irreversible! The encryption key is stored
in the file `fe.key`, so make sure to store it if you'd like to re-flash
that module later:

```bash
mos flash esp32 --esp32-encryption-key-file fe.key
```

Once the flash is encrypted, one can verify it using `flash-read` command
to ensure there no plain-text parts are present:

```bash
mos flash-read --arch esp32 0x190000 2000 -
```