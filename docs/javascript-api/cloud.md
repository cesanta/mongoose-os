---
title: Cloud
---

This interface provides an easy way to send data to the [Cesanta
cloud](https://cloud.cesanta.com/). On a cloud side, it is easy to build
interactive real-time dashboards.

- `Cloud.store(name, value [, options]) -> undefined`: Store metric `name` with
  value `value` in a cloud storage. Optional `options` object can be used to
  specify metrics labels and success callback function. Example:
  `Cloud.store('temperature', 36.6)`. The following prerequisites has to be
  met:
  - Wifi needs to be configured
  - Global configuration object `conf` needs to have device ID and password
    set, `conf.dev.id` and `conf.dev.psk`
  - Device with those ID and PSK needs to be registered in a cloud - see video
    at the top of this document

