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
mos -X esp32-gen-key flash_encryption_key fe.key \
	--esp32-enable-flash-encryption --dry-run=false
mos flash esp32 --esp32-encryption-key-file fe.key
```
That is irreversible - once flash encryption is enabled, you cannot go back.

Note the extra flag `--esp32-encryption-key-file fe.key`
for the `flash` command. From now on, a key file is required to re-flash the device.
If the key file is lost, the module can't be reflashed.
After flash encryption is enabled, the very first boot performs
an encryption, which takes a while - up to a minute.

Note the extra flag `--esp32-encryption-key-file fe.key`. Once the encryption
is enabled, the key file is required to re-flash the device. Make sure
to keep the key file, cause if it's lost, the module can't be reflashed.
After flash encryption is enabled, the very first boot actually performs
an encryption, which takes a while - up to a minute. Subsequent boots will
be normal, not doing any encryption.

Once the flash is encrypted, one can verify it using `flash-read` command
to ensure there no plain-text parts are present:

```bash
mos flash-read --arch esp32 0x190000 2000 -
```