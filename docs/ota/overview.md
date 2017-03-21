---
title: Overview
---

The Over-the-Air update process is designed to be a routine process,
i.e. it can be performed very frequently. The main feature of it is reliability.
Meaning, on any failure, a firmware is rolled back to the previous,
working state.

Mongoose OS firmware is .zip file that contains a binary code blob, and
a filesystem image. Device's flash memory is divided into two partitions,
or "slots". OTA update works this way:

- A new firmware gets downloaded to the currently inactive slot.
- A device gets rebooted.
- A boot loader boots from the new slot.
- A timer is started that checks for a special "commit" marker.
- If that commit marker is not set after the "commit timeout", then the device
  rolls back by rebooting into the old slot.
- If the commit marker is set, then the new slot is marked as active, and
  a new firmware is "blessed" from this point on.

Additionally, a new filesystem is created this way:

- A fresh empty filesystem is formatted.
- Files from the old filesystem are copied over.
- Files from the new firmware are copied on top, overwriting existing files.

Thus files that are present in the old filesystem but not present in the new
firmware, are preserved.

Mongoose OS configuration settings are stored in files. Default system
configuration are present in the firmware, whereas all user-defined overrides,
like WiFi, MQTT settings, etc, are stored in `conf.json` file which is not
shipped in a firmware image. Therefore, custom-made configuration overrides
are preserved during OTA updates.

The OTA can be triggered by:

- Calling an `OTA.Update` RPC, asking Mongoose to pull new firmware:
```bash
mos call OTA.Update '{
  "section": "firmware",
  "commit_timeout": 300,
  "blob_url": "http://MY-SITE/fw.zip"
}'
```

- Pushing new firmware directly by HTTP POST to a special `/update` endpoint:
```bash
curl -v -F file=@build/fw.zip -F commit_timeout=300 http://IP_ADDR/update
```

Notice the value of `commit_timeout`, 5 minutes. This is the commit timer
timeout mentioned above. If during that time a firmware is not marked as
fine by an explicit commit operation, it will be rolled back even if it is
working perfectly. To commit the new firmware, do:

```bash
mos call OTA.Commit
```

or

```bash
curl http://IP_ADDR/update/commit
```
