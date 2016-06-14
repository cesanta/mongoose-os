# Mongoose IoT Firmware boot loader for CC3200

`mg-boot.bin` should be loaded at `0x20004000` (this is where ROM loads `/sys/mcuimg.bin`).
It consists of a small relocator followed by the loader body.

The relocator, which is executed first, moves the loader body
to `0x2003C000` (256K - 16K) and executes it from there.

The loader's stack starts at `0x2003C000` and grows down.
Loader then proceeds to initialize NWP and read its configuration.

Configuration, in the form of `struct boot_cfg`, is read either from
`mg-boot.cfg.0` or `.1`. If both exist, whichever is more recent is used.
Recency is determined by comparing the `seq` counter. NB: config with _smaller_
`seq` is deemed to be more recent (counts down from `BOOT_CFG_INITIAL_SEQ`).

The config specifies which image to load next (`image_file`)
and where to (`base_address`). As usual, it is assumed that the image starts
with interrupt stack pointer followed by interrupt vector table.

Relocating loader to the end of RAM allows using the top 16 KB so application
images starting at `0x20000000` are perfectly fine.
Images up to approximately 240KB (256KB - 16KB - loader stack) can be loaded
this way.

Boot configs can be created with the `mkcfg` tool located in the `tools` dir:
```
  $ mkcfg $IMAGE_FILE $BASE_ADDRESS > boot.cfg.0
```
This initializes `seq` to `BOOT_CFG_INITIAL_SEQ`. If specifying `seq` is required:
```
  $ mkcfg $IMAGE_FILE $BASE_ADDRESS $SEQ > boot.cfg.0
```
