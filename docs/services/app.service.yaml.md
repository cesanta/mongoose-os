---
title: "App"
---

The App service manages apps inside a project.

#### Deploy
Deploy an app

Arguments:
- `path`: 
- `preview`: If true, the deploy is meant to only affect resources used by the developer, for development purposes.
- `target`: Deployment target. Keys and semantics depend on the type of the app
- `attrs`: App-kind specific deployment attributes.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/App.Deploy",
  "args": {
    "attrs": "VALUE PLACEHOLDER",
    "path": "VALUE PLACEHOLDER",
    "preview": "VALUE PLACEHOLDER",
    "target": "VALUE PLACEHOLDER"
  }
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


