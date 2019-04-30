# RPC support for Azure

## Direct Method RPC channel

Converts Azure [Direct Method](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-direct-methods) calls into mOS RPC calls.

Azure DM method name becomes RCP method name, payload becomes method args.

Since DM is a one way mechanism (cloud calls device), outgoing requests are rejected.

## Cloud Messaging

It sould be possible to support RPC over [cloud-to-device and device-to-cloud messaging](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messaging).

Unlike DM, it could be bi-directional, with device initiating the request.

This is no implemented yet.
