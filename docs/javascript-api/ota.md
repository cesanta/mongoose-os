---
title: Over-The-Air updates (OTA)
---

Mongoose IoT Platform's OTA is designed to work in both the unattended and controllable mode.

By default the updater module starts in unattended mode. Which means, it will
initiate the update process immediately after receiving a command from the server.
After a successful update, the device will be rebooted to use new firmware.

When in controllable mode, the updater invokes a callback on different stages
of the update process.  The callback can be set with the `Sys.updater.notify()`
function; its prototype is `function (event, params)`.  When the updater gets
an incoming request, the callback receives `Sys.updater.GOT_REQUEST` and
`params` contains the URL of the new firmware manifest file. The callback handler
can either start the update process with the `Sys.updater.start(url)` function or
delay this process, storing the provided URL and calling
`Sys.updater.start(url)` later.

Once the update process is finished, the callback receives one of the following
events:

- `Sys.updater.NOTHING_TODO`: the device is already running the newest firmware.
- `Sys.updater.FAILED`: the update cannot be completed, the callback handler can try to
  repeat updating by invoking Sys.updater.start(url) again.
- `Sys.updater.COMPLETED`: the update process completed successfully. The callback
  handler can reboot the device with `Sys.exit()` to use the new firmware
  immediately or delay reboot until a suitable moment arises.

Example:

```javascript
function upd(ev, url) {
   if (ev == Sys.updater.GOT_REQUEST) {
      print("Starting update from", url);
      Sys.updater.start(url);
   }  else if(ev == Sys.updater.NOTHING_TODO) {
      print("No need to update");
   } else if(ev == Sys.updater.FAILED) {
      print("Update failed");
   } else if(ev == Sys.updater.COMPLETED) {
     print("Update completed");
     Sys.reboot();
   }
}

Sys.updater.notify(upd);
```
