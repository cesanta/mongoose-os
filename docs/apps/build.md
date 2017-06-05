---
title: Building an App in C
---

Create an empty directory, go into it, and run:

```bash
mos init --arch ARCHITECTURE # cc3200, esp8266, esp32
```

That command creates a `mos.yml` configuration file with various
build settings, which you can alter later if needed. Also, a following
files and directories are created:

  - `src/`  - a directory for C source files.
  - `src/main.c` - a skeleton C source file with initialization function. You
    can add other files into this directory.
  - `fs/` - this directory will be copied verbatim to the device's
    filesystem. You can add files to this directory but make sure to stay
    within filesystem size boundaries.

Now, as all required files are created, we can build a firmware:

```sh
$ mos build
```

This command packs source and filesystem files and sends them to the
Mongoose OS cloud build backend. There is no need to setup and configure a
toolchain on a local machine! However, it is possible to do a local build.
In this case, your machine needs to have [Docker](https://www.docker.com/)
installed. To build locally, do

```sh
$ mos build --local --repo PATH_TO_CLONED_MONGOOSE_OS_REPO --verbose
```

A built firmware is stored in
the `build/fw.zip` file. Together with the built firmware, a couple of
other build artifacts are also stored in the `build/` directory:

- `build/fw.zip` - built firmware
- `build/fs` - a filesystem directory that is put in the firmware
- `build/gen` - generated header and source files

It is possible to set build flags to customize the build. Flags can be set
in `mos.yml` file, `build_vars` section. Here is the list of possible
flags, their meaning and their default values:

```yaml
build_vars:
  MGOS_DEBUG_UART: 0                  # Enable UART debugging
  MGOS_ENABLE_RPC: 1                  # Framing protocol for communication.
  MGOS_ENABLE_RPC_CHANNEL_UART: 1     # Needed for make mos tool to work.
  MGOS_ENABLE_CONFIG_SERVICE: 1       # Needed for mos config-* commands to work
  MGOS_ENABLE_DNS_SD: 1               # Enable network discovery
  MGOS_ENABLE_FILESYSTEM_SERVICE: 1   # Needed for mos ls,put,get to work
  MGOS_ENABLE_UPDATER: 1              # Enable OTA updates
  MGOS_ENABLE_UPDATER_RPC: 0          # Enable OTA via mg_rpc framing protocol
  MGOS_ENABLE_UPDATER_POST: 1         # Enable OTA via HTTP POST
```

3rd party C libraries could be included in the build in two different ways.
First, you can just copy library sources into some directory along with
`src/` directory, and add that directory to the list of source directories:

```yaml
sources:
  - src
  - my_library_directory
```

Second way is suitable for Git-managed libraries:

```yaml
sources:
  - src
  - ${mjs_path}
modules:
  - origin: https://github.com/cesanta/mjs
```

The example above includes an external library from Github, an embedded
scripting engine. Notice that other than specifying the external module origin,
we need to reference that module from `sources` and/or `filesystem` sections,
by using the `${}` syntax, otherwise there won't be any effect of that module.

Each external module has a name. By default it's equal to the last path
component of the origin, but it can also be overridden explicitly with the
`name` property, like this:

```yaml
modules:
  - name: mymjs
    origin: https://github.com/cesanta/mjs
```

For each module, `mos` tool creates a variable named `<modulename>_path`, where
`<modulename>` is a name of the module, with all non-alphanumeric characters
replaced with `_`.

Apart from explicit modules which you can include in your `mos.yml` file as
demonstrated above, there is one implicit module: `mongoose-os`, which refers
to the mongoose-os repository: https://github.com/cesanta/mongoose-os. Of
course, this module also has a corresponding path variable: `mongoose_os_path`
(notice that the dash `-` was replaced with the underscore here), and one could
take advantage of it as follows:

```yaml
sources:
  - src
  - ${mjs_path}
filesystem:
  - fs
  - ${mongoose_os_path}/fw/mjs_api/api_*.js
modules:
  - origin: https://github.com/cesanta/mjs
```

This is how a default Mongoose OS firmware is build, which includes mJS
scripting engine, see
https://github.com/cesanta/mongoose-os/tree/master/fw/examples/mjs_base 
