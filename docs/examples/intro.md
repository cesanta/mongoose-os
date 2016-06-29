---
title: Tutorials
items:
  - { name: blink.md }
  - { name: remote-control.md }
  - { name: send-cloud.md }
  - { name: send-restful.md }
  - { name: send-mqtt.md }
---

Firmware boot process executes `sys_init.js` file, which in turn executes
`app.js` file. The `app.js` file is empty by default, and it is supposed to
have custom device logic. Use Firmware JS API (below) section to see what
API is available to the firmware. Few examples are below.
