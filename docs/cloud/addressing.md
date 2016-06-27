---
title: Clubby addresses
---

In essence, Clubby is a very generic RPC protocol. It allows any two parties
with unique IDs to communicate. Thus, any device can talk to any
other device, or to any other entity (e.g. a cloud backend service).

When Clubby frame is constructed, `src` and `dst` fields specify source
and destination address, respectively. An address is an ID with
optional `.component` suffix. Component is sometimes needed to differentiate
separate communicating entities with the same ID - for example, image a user
logs in to Mongoose Cloud from two different machines. Her ID is a user ID,
but two browsers need separate addresses when talking to the cloud. Different
components are appended to the authenticated user ID to achieve that.

Architecturally, a central component of
[Mongoose Cloud](https://mongoose-iot.com) is a Dispatcher
service. Dispatcher listens on HTTPS and WSS ports for clubby frames,
looks inside each frame for the source and destination address,
asks Registry service to authenticate the request, and routes Clubby frames
to the correct destination.

When Mongoose Firmware boots, it creates a persistent secure Websocket
connection with the Dispatcher, making Dispatcher insert devices's ID
into the routing table. From that point on, any authenticated entity
can send commands to that device by connecting to the cloud (Dispatcher) and
crafting a Clubby request with device's ID as a destination address.

When a cloud backend service starts, it does the same - creates a persistent
connection with the Dispatcher, letting Dispatcher know its ID. A cloud
backend service also let Dispatcher know about methods it implements.
This makes it possible to omit destination address in the Clubby request.
If destination address is not present, then it is assumed to be one of the
cloud backend services. Dispatcher will figure out the destination address
by the method name.
