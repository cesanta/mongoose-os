---
title: "Dispatcher"
---

Commands provided by the dispatcher on the cloud backend.

#### Hello
The dispatcher learns about destinations reachable through a given channel by looking at the frames that come through them. This means that before receiving commands a device has to send at least one command. This method, mapped on /v1/Hello, offers a simple and cheap way to achieve this goal. Devices without a battery backed realtime clock will find the piggybacked server time to be useful.


Result `object`: 
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
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
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
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```

#### Help
Return basic info about the server


Result `object`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Dispatcher.Help",
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


