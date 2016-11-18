---
title: Boot process
---

Upon reboot or power on, Mongoose Firmware performs following steps:

1. System specific SDK initialisation

2. Filesystem initialisation

3. If the system configuration has a WiFi Access Point (AP) enabled
  (`Sys.conf.wifi.ap.enable`, enabled by default), a WiFi Access Point
  with the name `Mongoose_??????` is created. Question marks
  are replaced by the hex numbers from the device's MAC address.
  On this Access Point, the firmware runs a web server (covered in the next step)
  on IP address `192.168.4.1` with a configuration interface.

4. If the system configuration has web server enabled  (`Sys.conf.http.enable`
  option, enabled by default), a listening HTTP/WebSocket server starts on
  port `Sys.conf.http.listen_addr` (80 by default).

  A web server uses the `index.html` file on the filesystem to show a
  configuration UI and runs on IP address 192.168.4.1

  Also, users can programmatically add specific URI handlers to this
  server to implement a remote control.

  Note that the web server uses
  [Mongoose Embedded Web Server](https://github.com/cesanta/mongoose) as a networking engine.
  Mongoose supports many protocols. That means, a developer can use a system
  instance to send or receive requests in plain TCP, UDP, HTPP, WebSocket,
  MQTT or CoAP protocols.

3. (optional, if JavaScript support is enabled).
  Initialisation of the global JavaScript instance, `struct v7 *s_v7`.

  This instance is passed to the user-specific initialization function,
  in order to provide a way to initialize any custom objects in the
  JavaScript environment.

5. (optional, if JavaScript support is enabled).
  C code transfers control to the JavaScript by calling the `sys_init.js` file.
  This file:
   - Loads config files into the `Sys.conf` variable. This is done by
    loading `conf_sys_defaults.json`, then applying `conf_app_defaults.json`
    on top of it and then applying `conf.json` on top of it again.
   - Initialises connection to the cloud if required
    (`Sys.conf.mg_rpc.connect_on_boot` option). That allows talking
      to devices from anywhere, as the cloud relays messages to any connected
      devices.
   - Calls user-specific `app.js` file, which contains custom logic.

6. Systems executes infinite event loop.
