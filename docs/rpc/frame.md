---
title: RPC frame format
---

Request frame format:

```json
{
  "method": "Math.Add",
  "args": {
    "a": 1,
    "b": 2
  },
  "src": "joe/32efc823aa",
  "id": 1772
}
```

There is only one mandatory fields in the request, `method`. It specifies the
name of the service to call. All other fields are optional:

- `args` - arguments object
- `id` - numeric frame ID. Used to match responses with requests
- `src` - sender's ID. Required for channels like MQTT
- `tag` - any arbitrary string. Will be repeated in the response

Successful response frame format:

```json
{
  "result": { ... }
}
```

Failure response frame format:

```json
{
  "error": {
    "code": 123,
    "message": "oops"
  }
}
```

If the `error` key is present in the response, it's a failure. Failed
response may also contain a `result`, in order to pass more specific
information about the failure.
