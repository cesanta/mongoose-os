# A VFS device that encrypts reads and writes

## Overview

 * AES-128/192/256 are supported (`algo: AES-nnn` parameter, default is `AES-128`).
 * Reads and writes are encrypted, erases are passed through as is.
 * Encryption is performed in ECB mode, key is XORed with offset.
 * Reads and writes must be aligned to 16-byte boundaries.
 * Writes will be padded to 16 byte block size, so partial writes will only work for last plain-text block.

 _Hint:_ If you want an encrypted filesystem, [LFS](https://github.com/cesanta/mongoose-os/libs/vfs-fs-lfs) will work just fine with this method while [SPIFFS](https://github.com/cesanta/mongoose-os/libs/vfs-fs-spiffs) will not.

## Key source

Key can be supplied directly (as the `key` option) but a better approach is to use a _key device_ to obtain the key when required.

Key device can be any other VFS device that supports reads. It can be an existing device (`key_dev: name`) or created in-situ (`key_dev_type` + `key_dev_opts`).

_Hint:_ To read key from RAM, use the `vfs-dev-ram`.

_Hint 2:_ Want to generate your own key? Create your own VFS device. Don't worry about methods other than `read`.

## Example

Options for encrypting `extf0` with AES-256 with key from STM32 OTP area (536836096 = 0x1fff7800).

```json
 {"dev": "extf0", "algo": "AES-256", "key_dev_type": "RAM", "key_dev_opts": {"addr": 536836096, "size": 32}}

```

Don't forget to add `vfs-dev-ram` to libs.
