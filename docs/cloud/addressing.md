---
title: Clubby addresses
---

In essence, Clubby is a very generic RPC protocol. It allows any two parties
with unique IDs to communicate. Thus, any device can talk to any
other device or to any other entity (e.g. a cloud backend service).

When a Clubby frame is constructed, `src` and `dst` fields specify the source
and destination address, respectively. An address is an ID with
optional `.component` suffix. A component is sometimes needed to differentiate
separate communicating entities with the same ID. For example, imagine a user
logs in to Mongoose Cloud from two different machines. Her ID is a user ID,
but the two browsers need separate addresses when talking to the cloud. Different
components are appended to the authenticated user ID to achieve that.

Architecturally, a central component of
[Mongoose Cloud](https://mongoose-iot.com) is a Dispatcher
service. The Dispatcher listens on HTTPS and WSS ports for Clubby frames,
looks inside each frame for the source and destination address,
asks the Registry service to authenticate the request and routes Clubby frames
to the correct destination.

When Mongoose Firmware boots, it creates a persistent secure WebSocket
connection with the Dispatcher, making Dispatcher insert the devices's ID
into the routing table. From that point on, any authenticated entity
can send commands to that device by connecting to the cloud (Dispatcher) and
crafting a Clubby request with the device's ID as a destination address.

When a cloud backend service starts, it does the same. It creates a persistent
connection with the Dispatcher, letting the Dispatcher know its ID. A cloud
backend service also lets Dispatcher know about the methods it implements.
This makes it possible to omit the destination address in the Clubby request.
If a destination address is not present, then it is assumed to be one of the
cloud backend services. Dispatcher will figure out the destination address
by the method name.
