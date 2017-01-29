---
title: Overview
---

Mongoose OS WiFi provides a Remote Procedure Call API that makes device
accessible remotely. The API allows to:

- Create an RPC service. In a nutshell, this is C function that is given
  an RPC endpoint name, and gets triggered when a request for that
  endpoint arrives in one of the supported channels (serial, HTTP, MQTT, etc)
- Call RPC service. Allows to call a given endpoint remotely - for example,
  another device

Mongoose OS RPC requests (MG-RPC) and responses are marshalled into JSON frames,
which are very similar to JSON-RPC. The difference is that MG-RPC adds
some extra optional keys.

For an example, take a look at
[c_rpc](https://github.com/cesanta/mongoose-os/tree/master/fw/examples/c_rpc)
example firmware that implements `Example.Increment` RPC service.
