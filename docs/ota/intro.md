---
title: "OTA (over-the-air programming)"
---

Smart.js OTA is designed to provide ability to update firmware on the device
over Internet, without connecting it to computer and using Flash'n'Chips.

In order to use the following steps the device must be registered in
Cesanta Cloud (see Quick Start Guide for details).

Open https://dashboard.cesanta.com/#/ota in your favourite browser.

<img src="../../static/img/smartjs/ota1.png" align="center"/>

Then press "select .zip archive", point zip-file with new firmware and then
press "Upload new firmware".
The firmware will be uploaded to server, it may take a while.

<img src="../../static/img/smartjs/ota2.png" align="center"/>

Next step is a creation of rollout. Rollout is a set of devices, which
must use the same firmware.
Fill "Rollout name", then choose firmware from dropdown list, and fill
"Filter" field. For example, if you want to apply new firmware to all
esp8266 in the project, use `arch=esp8266` as a filter.
Press "Add new rollout".

<img src="../../static/img/smartjs/ota3.png" align="center"/>

Once rollout is created, server starts to send notification about new firmware
to devices. The device can be offline, at the rollout creation moment, it will
receive notification when it will come online.

You can monitor update status in "State" field.
`inProgress` means that server is sending notifications.
`finished` means, that all noitifications was sent.

Once `State` becomes `finished` you can open https://dashboard.cesanta.com/#/devices,
click link in `Id` column and check if `fw_version` field contains
new version.

<img src="../../static/img/smartjs/ota4.png" align="center"/>

One of the important features of a reliable OTA implementation is to make sure
the device never freezes after an unsuccessful firmware update. There could be
plenty of reasons why an update could fail, that’s why a reliable implementation
should always roll back to the previous firmware in case of failure.

We have tackled this by utilising several components on the firmware side:

- Flexible boot loader
- Firmware self-checking procedure

A new firmware is loaded into a separate section (partition) on flash. Then, the
firmware is rebooted, and a boot loader boots a new section, noting this in a
special flag on flash drive.

After the boot, the new firmware performs a self-check. A successful self-check
means that the new firmware is working as expected, and in this case the new
firmware updates a special flag on a flash drive.

If the new firmware failed to self-check and rebooted, or if it froze and
watchdog rebooted the module, a boot loader notes this also with a special flag
specifying that the update was not completed and boots an old partition.
