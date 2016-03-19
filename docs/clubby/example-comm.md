---
title: Example communication
---

Device submits data to the cloud with `/v1/Metrics.Publish` command:

```json
{
  "v": 1,
  "src": "//api.cesanta.com/d/123",
  "dst": "//api.cesanta.com/p/123",
  "cmds": [
    {
      "cmd": "/v1/Metrics.Publish",
      "id": 1237648172,
      "args": {
        "vars": [
          [{"__name__": "temperature", "sensor": "1"}, 28]
        ]
      }
    }
  ]
}
```

Cloud sends back a response and a command for the device:

```json
{
  "v": 1,
  "src": "//api.cesanta.com/p/123",
  "dst": "//api.cesanta.com/d/123",
  "resp": [
    {
      "id": 1237648172,
      "status": 0
    }
  ],
  "cmds": [
    {
      "cmd": "/v1/LED.IsOn",
      "id": 214232345
    }
  ]
}
```

Device responds back:

```json
{
  "v": 1,
  "src": "//api.cesanta.com/d/123",
  "dst": "//api.cesanta.com/p/123",
  "resp": [
    {
      "id": 214232345,
      "status": 0,
      "response": true
    }
  ]
}
```

