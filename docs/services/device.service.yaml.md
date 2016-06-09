---
title: "Device"
---

Provides methods to manage devices.

#### Claim
Claim an unclaimed device using a token.

Arguments:
- `projectid`: ID of the project.
- `token`: Auth token based on PSK.
- `deviceid`: ID of the device.

#### Add
Adds a new device.

Arguments:
- `psk`: Pre-shared key that device will use for authentication.
- `deviceid`: ID of the device.
- `projectid`: Optional ID of the project to add device to.

#### GetInfo
Gets device information.

Arguments:
- `deviceid`: ID of the device.

Result `object`: 
#### List
Returns a list of devices in a given project.

Arguments:
- `filter`: Filter expression, currently only 'labelname=labelvalue' supported.
- `projectid`: ID of the project.

Result `array`: 
#### Delete
Removes the devices from the project.

Arguments:
- `projectid`: ID of the project.
- `deviceid`: ID of the device.


