---
title: Boot process
---

Upon reboot or power on, Mongoose OS performs following steps:

1. System specific SDK initialisation
2. Filesystem initialisation
3. If the system configuration has a WiFi Access Point (AP) enabled
  (`wifi.ap.enable` set to `true`, which is a default), a WiFi Access Point
  with the name `Mongoose_??????` is created. Question marks
  are replaced by the hex numbers from the device's MAC address.
  On this Access Point, the Mongoose OS runs a web server
  on IP address `192.168.4.1`, serving `index.html` file that can
  be customised.
4. If `wifi.sta.enable` is set to `true`, then Mongoose OS joins the existing
  WiFi network, using network name `wifi.sta.ssid` and password `wifi.sta.pass`.
5. A system web server mentioned previously is configured
  by `http.enable`, set to `true` by default. It can serve files from a
  filesystem. Also, it handles Websocket connections (used by RPC), and
  handles couple of other endpoints: file uploader, and firmware updater.
  Run `mos config-get http` to see other configuration options.
6. MQTT server connection is established if configured so, see `mqtt` top
  level configuration
7. Mongoose OS starts an RPC subsystem by attaching enabled RPC services
  to serial, HTTP, Websocket, MQTT transports. This makes it possible to
  call various device functionality remotely, like manipulating GPIO pins,
  reading/writing from I2C, working with files, updating the firmware
  over-the-air, and so on. User can write custom RPC services.
8. Mongoose OS calls user's initialisation function, `mgos_app_init()`,
  defined in `YOUR_FIRMWARE_DIR/src/main.c`. That function is an entry
  point for all custom code. It should return either `MGOS_APP_INIT_SUCCESS`
  or `MGOS_APP_INIT_ERROR`. If error is returned, then the firmware is
  regarded as faulty and is rolled back, if updated over the air.
9. Systems executes infinite event loop.
