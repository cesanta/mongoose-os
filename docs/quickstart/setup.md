---
title: Setup mos tool
---

Download `mos` tool from https://mongoose-iot.com/software.html
On Linux and Mac, make it executable after the download:

```bash
cp mos ~/bin/
chmod 755 ~/bin/mos
```

`mos` can run a simple Web UI wizard, which runs automatically if `mos`
is not started from a terminal. You can fore the UI by setting `--ui` flag,
or disable the UI by settings `--ui=false` flag.

`mos` tool is self-documented. Run it without arguments to shows a help
string. Run `mos help <command>` to see a help string for that particular
command.

Export environment variables - on Mac/Linux, put these into your `~/.profile`:

```bash
export MOS_PORT=YOUR_SERIAL_PORT  # E.g. /dev/ttyUSB0 on Linux
```

On Windows, start `cmd.exe` command line prompt and do:

```
set MOS_PORT=YOUR_SERIAL_PORT  # E.g. COM6 on Windows
```

You can set a default value for any `mos` flag through the environment
variables, just put `export MOS_FLAGNAME=VALUE` into your `~/.profile`.
