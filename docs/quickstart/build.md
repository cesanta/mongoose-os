---
title: Build the firmware
---

Download `miot` command line tool from https://mongoose-iot.com/software.html .
Create an empty directory, go into it, and run:

```bash
miot init --arch ARCHITECTURE # cc3200 or esp8266
```

That command creates a `mg-app.yaml` configuration file with various
build settings, which you can alter later if needed. Also, a following
files and directories are created:

  - `src/`  - a directory for C source files.
  - `src/app.c` - a skeleton C source file with initialization function. You
    can add other files into this directory.
  - `fs/` - this directory will be copied verbatim to the device's
    filesystem. You can add files to this directory but make sure to stay
    within filesystem size boundaries.

Now, as all required files are created, we can build a firmware:

```sh
$ miot build --user YOUR_USERNAME --pass YOUR_PASSWORD
```

This command packs source and filesystem files and sends them to the
Mongoose Cloud. There, a firmware builder backend builds
a firmware for the specified architecture and sends the result back.
A built firmware is stored in
the `build/fw.zip` file. Together with the built firmware, a couple of
other build artifacts are also stored in the `build/` directory:

- `build/fw.zip` - built firmware
- `build/fs` - a filesystem directory that is put in the firmware
- `build/gen` - generated header and source files
