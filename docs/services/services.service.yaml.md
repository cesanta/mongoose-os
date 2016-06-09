---
title: "Services service"
---

Services service provides support for introspection.

#### List
Get returns definitions of all services provided by the server.


Result `object`: Property names are full service names (prefixed with namespace) and values - their definitions.
Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/device_123",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/Services.List",
      "id": 123
    }
  ]
}

```


