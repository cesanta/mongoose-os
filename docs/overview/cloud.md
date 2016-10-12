---
title: Cloud services
---

[Mongoose Cloud](https://mongoose-iot.com) provides a set
of services that provide basic infrastructure functionality for creating
connected products: The list include:

- Auth: authentication and fine-grained access control service
- Registry: Keep track of registered users, devices
  and their metadata.
- Timeseries: Store telemetry data reported by devices - for example values
  reported by various sensors, performance statistics data, and so on.
  This service also provides a rich set of analytics functions.
- Publish/Subscribe: Used to push real-time data to a large number of
  subscribers.
- Log: Send and store logs from devices.
- KV: Key/value store for arbitrary files, configuration data, etc.

Mongoose Cloud is designed to be easily extendable. You can create your
own backend in any language or framework you want, register it with
Mongoose Cloud, and instantly benefit from authentication and access control
features Mongoose Cloud provides. Also your backend gets access to all data
stored in the Mongoose Cloud.

If your custom backend is developed on the 3rd party platform such as
Amazon IoT, Microsoft Azure or other - you can benefit from the both worlds:
for example, use management capabilities of the Mongoose Cloud and use
data analytics capabilities of the 3rd party platform.
