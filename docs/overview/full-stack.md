---
title: Full-stack platform
---

IoT connects things (devices) and the Internet (cloud) together.
A software that sits on devices and a cloud to provide that connectivity
and integration is called an IoT platform.

<img src="media/over_full_stack.png" width="100%">

Mongoose IoT is a full-stack platform. For the device side, there is
Mongoose Firmware. For the cloud part, there is Mongoose Cloud.
Cesanta runs a managed [Mongoose Cloud](https://mongoose-iot.com/cloud.html)
public service, available for everyone to use.
Alternatively, Mongoose Cloud can be run privately -
[contact us](https://mongoose-iot.com/contact.html) for details.

Mongoose Firmware targets 32-bit microprocessors. Typical requirements
are 40+ kilobytes RAM, and 128+ kilobytes flash. Note that for
Over-The-Air (OTA) updates, flash memory has to accommodate two firmware
images, therefore boards with smaller flash memory might be able to run
Mongoose Firmware but not be able to update themselves remotely.

Mongoose Firmware and Mongoose Cloud are independent components and could
be used either together or separately.
