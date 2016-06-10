---
title: "Frontend"
---

Misc APIs provided by the HTTP frontend.

#### BlobURL
Returns an HTTP URL which points to the given blobstore key.

Arguments:
- `key`: Blobstore key.

Result `string`: A URL.
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Frontend.BlobURL",
  "args": {
    "key": "VALUE PLACEHOLDER"
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


