---
title: "Cloud Overview"
items:
  - { type: file, name: frame-structure.md }
  - { type: file, name: addressing.md }
  - { type: file, name: commands-and-responses.md }
  - { type: file, name: example-comm.md }
  - { type: file, name: auth.md }
  - { type: file, name: init.md }
  - { type: file, name: req.md }
  - { type: file, name: custom_msg.md }
---

We use a special JSON-based protocol to communicate with the cloud: clubby.
Clubby operates on frames that can be carried by multiple transports: raw TCP
connection, HTTP POST request or WebSocket.

In order to make development of client applications even easier, we maintain
Clubby and Cloud libraries in a number of languages.

For generated API documentation, please follow:

- [Generated JavaDoc for Cloud library](/cloud_java/latest). Cloud library
  includes the Clubby implementation, plus the helper stubs for accessing the
  cloud. More on that later.
