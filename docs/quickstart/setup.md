---
title: Setup miot
---

Download `miot` tool from https://mongoose-iot.com/software.html
On Linux and Mac, make it executable after the download:

```bash
cp miot ~/bin/
chmod 755 ~/bin/miot
```

`miot` tool is self-documented. Run it without arguments to shows a help
string. Run `miot help <command>` to see a help string for that particular
command.

Export environment variables - on Mac/Linux, put these into your `~/.profile`:

```bash
export MIOT_PORT=YOUR_SERIAL_PORT  # E.g. /dev/ttyUSB0 on Linux
```

On Windows, start `cmd.exe` command line prompt and do:

```
set MIOT_PORT=YOUR_SERIAL_PORT  # E.g. COM6 on Windows
```

You can set a default value for any `miot` flag through the environment
variables, just put `export MIOT_FLAGNAME=VALUE` into your `~/.profile`.
