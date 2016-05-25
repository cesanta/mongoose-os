---
title: "Clubby protocol"
items:
  - { type: file, name: frame-structure.md }
  - { type: file, name: addressing.md }
  - { type: file, name: commands-and-responses.md }
  - { type: file, name: example-comm.md }
---

We use a special JSON-based protocol to communicate with the cloud: clubby.
Clubby operates on frames that can be carried by multiple transports: raw TCP
connection, HTTP POST request or WebSocket.
