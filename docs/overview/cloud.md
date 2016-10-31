---
title: Mongoose Cloud
---

[Mongoose Cloud](https://mongoose-iot.com/cloud.html) provides basic
functionality for creating connected products:

- Authentication and fine-grained access control. For example, you can
  tell grant or restrict access to a particular device or its data
- Registry of devices and their metadata: any device can be annotated
  with arbitrary key/value labels. For example, you can assign
  `city=Dublin` and `city=London` labels  to your devices, and roll different
  firmware depending on a device label
- Timeseries database for storing values
  reported by various sensors, performance statistics data, and so on.
- Configurable dashboards with graphs that show reported values.
- Event logging service
- Rules engine that can set thresholds on a reported values and generate
  events with specified severity
- Email notifications can send you emails when events with certain severity
  happen
- Over-the-air (OTA) firmware updates or configuration updates

Mongoose Cloud is designed to be easily extendable. You can create your
own backend in any language or framework you want, register it with
Mongoose Cloud, and instantly benefit from authentication and access control
features Mongoose Cloud provides. Also your backend gets access to all data
stored in the Mongoose Cloud.

If your custom backend is developed on the 3rd party platform such as
Amazon IoT, Microsoft Azure or other - you can benefit from the both worlds:
for example, use management capabilities of the Mongoose Cloud and use
data analytics capabilities of the 3rd party platform.
