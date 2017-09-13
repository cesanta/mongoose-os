---
title: Over-the-air (OTA) update
---

It's time to show how the OTA update works.

Make some changes to the `src/main.c` and build the firmware. Push new
firmware to the device using simple `curl` command:

```
curl -F @build/fw.zip http://DEVICE_IP/update
```

For this to work, a device needs to be directly accessible.

<!--
Alternatively,
a firmware could be uploaded to some web server of your choice:
http://SERVER/path/to/fw.zip , then the device can
be instructed to update from that location:

```
curl http://mongoose.cloud/api/YOUR_USERNAME.DEVICE_ID/commands/update \
  -u YOUR_USERNAME:YOUR_PASSWORD \
  -d '{"url":"https://SERVER/path/to/fw.zip"}'
```
-->
