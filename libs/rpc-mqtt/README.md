# Implementation of Mongoose OS RPC over MQTT protocol

## Overview

MQTT RPC channel allows invoking RPC calls via MQTT. For example, assuming
your device id is `esp8266_DA7E15`, this command could be invoked to get
the config of a remote device:

```bash
mos --port mqtt://iot.eclipse.org:1883/esp8266_DA7E15 call Config.Get
```
