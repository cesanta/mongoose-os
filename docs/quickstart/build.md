---
title: Build the firmware
---

Download `miot` command line tool from https://mongoose-iot.com/downloads.html .
Create an empty directory, go into it, and execute this in that directory:

```sh
$ miot fw init --arch esp8266
```

That command creates a `mg-app.yaml` configuration file with various
build settings, which you can alter later at your will. Also, a following
files and directories are created:

  - `src/`  - a directory for C source files.
  - `src/app.c` - a skeleton C source file with initialization function. You
    can add other files into this directory
  - `filesystem/` - this directory will be copied verbatim to the device's
    filesystem. You can add files to this directory at your will

When all necessary files are created, build a firmware:

```sh
$ miot cloud build
```

This command packs all source files, filesystem files, and sends them to the
cloud. On the cloud, a builder backend builds a firmware for a specified
architecture, and sends the result back. A built firmware is stored in
the `build/fw.zip` file.
