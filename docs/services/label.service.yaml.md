---
title: "Label service"
---

Allows to manage arbitrary string labels for devices.

#### Get
Returns labels set on a particular devices.

Arguments:
- `labels`: Optional list of labels to fetch. If not set, all labels will be returned.
- `ids`: List of device IDs to fetch labels for.

Result `array`: 
Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/user_123",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/Label.Get",
      "id": 123,
      "args": {
        "ids": ["//api.cesanta.com/device_123"],
        "labels": ["location"]
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
  "dst": "//api.cesanta.com/user_123",
  "resp": [
    {
      "id": 123,
      "status": 0,
      "resp": [
        {
          "id": "//api.cesanta.com/device_123",
          "labels": {
            "location": "Bathroom"
          }
        }
      ]
    }
  ]
}

```

#### Set
Sets labels for devices.

Arguments:
- `labels`: An object with labels to set. Object keys are label names, corresponding values are label values to set.
- `ids`: List of device IDs to set labels for.

Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/user_123",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/Label.Set",
      "id": 123,
      "args": {
        "ids": ["//api.cesanta.com/device_123"],
        "labels": {
          "location": "Bathroom"
        }
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
  "dst": "//api.cesanta.com/user_123",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
}

```

#### Delete
Deletes labels for devices.

Arguments:
- `labels`: List of names of labels to delete.
- `ids`: List of device IDs to delete labels for.

Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/user_123",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/Label.Delete",
      "id": 123,
      "args": {
        "ids": ["//api.cesanta.com/device_123"],
        "labels": ["location"]
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
  "dst": "//api.cesanta.com/user_123",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
}

```


