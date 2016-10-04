---
title: Build the firmware
---

Download `miot` command line tool from https://mongoose-iot.com/software.html .
Create an empty directory, go into it, and run:

```sh
$ miot fw init --arch esp8266
```

Note if you're using CC3200 board, use `--arch cc3200`.

That command creates a `mg-app.yaml` configuration file with various
build settings, which you can alter later if needed. Also, a following
files and directories are created:

  - `src/`  - a directory for C source files.
  - `src/app.c` - a skeleton C source file with initialization function. You
    can add other files into this directory.
  - `filesystem/` - this directory will be copied verbatim to the device's
    filesystem. You can add files to this directory but make sure to stay
    within filesystem size boundaries.

Now, as all required files are created, we can build a firmware:

```sh
$ miot cloud build
```

This command packs source and filesystem files and sends them to the
cloud. There, firmware builder backend builds a firmware for a specified
architecture and sends the result back. A built firmware is stored in
the `build/fw.zip` file. Together with the built firmware, a couple of
build artifacts are also stored in the `build/` directory:

- `build/fs` - a filesystem directory that is put in the firmware
- `build/gen` - generated header and source files
