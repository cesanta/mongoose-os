---
title: "Dispatcher"
---

Commands provided by the dispatcher on the cloud backend.

#### Hello
A simple echo service.


Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Dispatcher.Hello",
  "args": {}
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.cesanta.com",
  "dst": "device_123",
  "id": 123
}

```

#### RouteStats
Gets channel stats for one or more IDs.

Arguments:
- `ids`: List of ids to query

Result `object`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Dispatcher.RouteStats",
  "args": {
    "ids": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.cesanta.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```


