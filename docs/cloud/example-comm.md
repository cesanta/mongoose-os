---
title: Example communication
---

Device submits data to the cloud with `/v1/Timeseries.ReportMany` command:

```json
{
  "v": 2,
  "src": "//api.cesanta.com/d/123",
  "dst": "//api.cesanta.com/p/123",
  "id": 1237648172,
  "method": "/v1/Timeseries.ReportMany",
  "args": {
    "vars": [
      [{"__name__": "temperature", "sensor": "1"}, 28]
    ]
  }
}
```

Cloud sends back a response:

```json
{
  "v": 2,
  "src": "//api.cesanta.com/p/123",
  "dst": "//api.cesanta.com/d/123",
  "id": 1237648172
}

There is no result and no error, meaning invocation completed successfuly.
```

An error would be returned like this:
```json
{
  "v": 2,
  "src": "//api.cesanta.com/p/123",
  "dst": "//api.cesanta.com/d/123",
  "id": 1237648172,
  "error": {
    "code": 123,
    "message": "Something went wrong"
  }
}
```
