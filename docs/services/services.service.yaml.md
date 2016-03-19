---
title: "Services service"
---

Services service provides support for introspection.

#### Get
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
      "cmd": "/v1/Services.Get",
      "id": 123
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com",
  "dst": "//api.cesanta.com/device_123",
  "resp": [
    {
      "id": 123,
      "status": 0,
      "resp": {
        "http://cesanta.com/clubby/service/v1/Services": {
          "name": "/v1/Services",
          "namespace": "http://cesanta.com/clubby/service",
          "doc": "Services service provides support for introspection.",
          "methods": {
            "Get": {
              "doc": "Get returns definitions of all services provided by the server.",
              "result": {
                "additionalProperties": {
                  "$ref": "http://cesanta.com/clubby/schema/service/v1#"
                },
                "doc": "Property names are full service names (prefixed with namespace) and values – their definitions.",
                "type": "object"
              }
            }
          }
        }
      }
    }
  ]
}

```


