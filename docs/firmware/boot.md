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

5. Systems executes infinite event loop.
