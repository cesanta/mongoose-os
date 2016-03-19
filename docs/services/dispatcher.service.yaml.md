---
title: "Dispatcher service"
---

Commands provided by the dispatcher on the cloud backend.

#### Hello
A simple echo service.



Definition:
```json
{
  "doc": "A simple echo service."
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
      "cmd": "/v1/Dispatcher.Hello",
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
      "resp": "HAI"
    }
  ]
}

```

#### RouteStats
Gets channel stats for one or more IDs.

Arguments:
- `ids`: List of ids to query

Result `object`: 

Definition:
```json
{
  "doc": "Gets channel stats for one or more IDs.",
  "args": {
    "ids": {
      "doc": "List of ids to query",
      "type": "array",
      "items": {
        "type": "string"
      }
    }
  },
  "required_args": [
    "ids"
  ],
  "result": {
    "additionalProperties": {
      "required": [
        "created",
        "lastUsed",
        "numSent",
        "channels"
      ],
      "type": "object",
      "properties": {
        "channels": {
          "doc": "Channels associated with this route",
          "type": "array",
          "items": {
            "type": "string"
          }
        },
        "created": {
          "doc": "When the route was created (timestamp, seconds)",
          "type": "integer"
        },
        "numSent": {
          "doc": "Number of frames sent to the route",
          "type": "integer"
        },
        "lastUsed": {
          "doc": "When the route was last used (frame sent of received; timestamp, seconds)",
          "type": "integer"
        }
      }
    },
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
      "cmd": "/v1/Dispatcher.RouteStats",
      "id": 123,
      "args": {
        "ids": ["//api.cesanta.com/device_123"]
      }
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
        "//api.cesanta.com/device_123": {
          "created": 1453395062,
          "lastUsed": 1453395062,
          "numSent": 10,
          "channels": [
            "[websocket from 10.99.12.5:23847]"
          ]
        }
      }
    }
  ]
}

```


