---
title: Overview
---

Smart.js is a generic, cross-platform, full-stack
Internet of Things software platform. It makes it fast and easy to connect
devices online. For the device part, Smart.js provides JavaScript-enabled
firmware. Device part is integrated with the cloud part, which provides
device management, OTA (Over The Air) reliable update functionality, and
designed for easy integration with 3rd party database and analytics software.

Smart.js provides an API to talk to 3rd party cloud services through
protocols like HTTP, WebSocket, MQTT, etc. Commonly, developers invent
their own data representation formats to communicate data and commands
to their devices. For example, if some device needs to respond to user commands
sent from the mobile phone, commands are wrapped into frames, transferred
by HTTP/HTTPS or other protocol to the device, which then interprets them.
Authentication, device addressability, availability, extensibility of the
approach, and many other things need to be taken care of.

To solve that common task, at Cesanta we created a framing format called Clubby.
It defines point-to-point, secure communication that tolerates prolonged
device disconnections. It could use human-readable JSON, or compact binary
UBJSON encodings. With the help our libraries, it makes it easy to implement
data transfer and remote device control. Clubby is documented in detail
below, allowing alternative client or server implementations in any
programming language.

Devices that run Smart.js can use any cloud infrastructure. Usual tasks include
reporting some measurements from devices to the cloud, visualizing that data,
and device management & remote software update. Cesanta offers ready-to-go
cloud infrastructure for all that, available as a cloud service
at link:https://cloud.cesanta.com[], or as a software that can be run privately.
A quick start guide below shows simple steps on how to bootstrap an IoT
device running Smart.js firmware, and hook it to the cloud service.

Smart.js solves problems of reliability, scalability, security
and remote management which are common to all verticals, being it industrial
automation, healthcare, automotive, home automation, or other.

Take a look at 2 minute video that shows Smart.js in action:

[ ![Smart.js](https://docs.cesanta.com/images/Smart.js.clip.png) ](https://www.youtube.com/watch?v=6DYfGsqQzCg)

At the moment, Smart.js supports following hardware platforms:

- Espressif ESP8266
- POSIX - all POSIX compatible systems, including:
  * Raspberry PI
  * BeagleBone
  * Linux
  * Windows
  * MacOS
