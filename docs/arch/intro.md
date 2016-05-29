---
title: "Architecture"
---

Commonly, developers invent
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

<img src="media/Mongoose_IoT_diagram.png" align="center" width="100%"/>

Mongoose IoT firmware on a device side:

- Allows scripting for fast and safe development & firmware update. We do that
  by developing [V7 - the world's smallest JavaScript
  engine](https://github.com/cesanta/v7).
- Provides hardware and networking API that guarantees reliability, scalability
  and security out-of-the-box.
- Devices with our software can be managed remotely and update software
  remotely, in a fully automatic or semi-automatic way.
