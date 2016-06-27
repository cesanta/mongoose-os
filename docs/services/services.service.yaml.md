---
title: "Services"
---

Services service provides support for introspection.

#### List
Get returns definitions of all services provided by the server.


Result `object`: Property names are full service names (prefixed with namespace) and values - their definitions.
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Services.List",
  "args": {}
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```


