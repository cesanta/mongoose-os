---
title: "Building Smart.js firmware"
---

For those who want to build Smart.js firmware themselves, here is an
instruction on how to do that:

1. Make sure you have [Docker](https://www.docker.com/) installed and running
2. Execute the following:

```
$ git clone https://github.com/cesanta/smart.js.git
$ cd smart.js/platforms/esp8266
$ make
```

The firmware gets built in the `firmware/` folder. Copy it to the
Fish'n'chips's `firmware/esp8266/` directory, and it'll be ready to flash!

NOTE: for Windows users:

- Operations above should be performed from `boot2docker` console, not from `cmd.exe`
- Only `C:\Users` is mapped into docker's VM by default, so put cloned repo to `Users` subfolder

NOTE: for Mac users:

- Only `/Users` is mapped into docker's VM by default, so put cloned repo to `Users` subfolder
