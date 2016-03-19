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

Definition:
```json
{
  "doc": "Returns labels set on a particular devices.",
  "args": {
    "labels": {
      "doc": "Optional list of labels to fetch. If not set, all labels will be returned.",
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "ids": {
      "minItems": 1,
      "doc": "List of device IDs to fetch labels for.",
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
    "items": {
      "type": "object",
      "properties": {
        "labels": {
          "additionalProperties": {
            "type": "string"
          },
          "type": "object"
        },
        "id": {
          "type": "string"
        }
      }
    },
    "type": "array"
  }
}
```

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


Definition:
```json
{
  "doc": "Sets labels for devices.",
  "args": {
    "labels": {
      "additionalProperties": {
        "type": "string"
      },
      "doc": "An object with labels to set. Object keys are label names, corresponding values are label values to set.",
      "type": "object"
    },
    "ids": {
      "minItems": 1,
      "doc": "List of device IDs to set labels for.",
      "type": "array",
      "items": {
        "type": "string"
      }
    }
  }
}
```

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


Definition:
```json
{
  "doc": "Deletes labels for devices.",
  "args": {
    "labels": {
      "doc": "List of names of labels to delete.",
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "ids": {
      "minItems": 1,
      "doc": "List of device IDs to delete labels for.",
      "type": "array",
      "items": {
        "type": "string"
      }
    }
  }
}
```

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


