---
title: Setup mgos
---

Download `mgos` tool from https://mongoose-iot.com/software.html
On Linux and Mac, make it executable after the download:

```bash
cp mgos ~/bin/
chmod 755 ~/bin/mgos
```

`mgos` tool is self-documented. Run it without arguments to shows a help
string. Run `mgos help <command>` to see a help string for that particular
command.

Export environment variables - on Mac/Linux, put these into your `~/.profile`:

```bash
export MGOS_PORT=YOUR_SERIAL_PORT  # E.g. /dev/ttyUSB0 on Linux
```

On Windows, start `cmd.exe` command line prompt and do:

```
set MGOS_PORT=YOUR_SERIAL_PORT  # E.g. COM6 on Windows
```

You can set a default value for any `mgos` flag through the environment
variables, just put `export MGOS_FLAGNAME=VALUE` into your `~/.profile`.
