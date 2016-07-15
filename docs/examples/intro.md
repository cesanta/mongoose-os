---
title: Tutorials
items:
  - { name: blink.md }
  - { name: remote-control.md }
  - { name: send-cloud.md }
  - { name: send-restful.md }
  - { name: send-mqtt.md }
---

The firmware boot process executes `sys_init.js` file, which in turn executes the
`app.js` file. The `app.js` file is empty by default and it is supposed to
have custom device logic. Use the Firmware JS API (section below) to see what
API is available to the firmware. A few examples are shown below.
