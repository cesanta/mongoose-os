---
title: "Services service"
---

Services service provides support for introspection.

#### List
Get returns definitions of all services provided by the server.


Result `object`: Property names are full service names (prefixed with namespace) and values - their definitions.

Definition:
```json
{
  "doc": "Get returns definitions of all services provided by the server.",
  "result": {
    "additionalProperties": {
      "$ref": "http://cesanta.com/clubby/schema/service/v1#",
      "keep_as_json": true
    },
    "doc": "Property names are full service names (prefixed with namespace) and values - their definitions.",
    "type": "object"
  }
}
```

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


