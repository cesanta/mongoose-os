# Location library

## Overview

Provides a function to get latitude and longtitude; so far it merely
returns the configured values. Example usage, in the app's `mos.yml`:

```yaml
libs:
  - origin: https://github.com/cesanta/mongoose-os/libs/location

config_schema:
  - ["device.location.lat", 53.3242381]
  - ["device.location.lon", -6.385785]
```
