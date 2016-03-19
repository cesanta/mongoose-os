---
title: "Metrics service"
---

Metrics service provides timeseries storage.

#### Query
Query returns the result of executing `query` on the data stored. `query` uses Prometheus query language.

Arguments:
- `query`: Query to execute.

Result `array`: 

Definition:
```json
{
  "doc": "Query returns the result of executing `query` on the data stored. `query` uses Prometheus query language.",
  "args": {
    "query": {
      "doc": "Query to execute.",
      "type": "string"
    }
  },
  "result": {
    "items": {
      "minItems": 2,
      "items": [
        {
          "additionalProperties": {
            "type": "string"
          },
          "type": "object"
        },
        {
          "items": {
            "items": [
              {
                "doc": "Timestamp.",
                "type": "integer"
              },
              {
                "doc": "Value.",
                "type": "number"
              }
            ],
            "type": "array"
          },
          "type": "array"
        }
      ],
      "additionalItems": false,
      "type": "array"
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
      "cmd": "/v1/Metrics.Query",
      "id": 123,
      "args": {
        "query": "{id=\"//api.cesanta.com/device_123\"}"
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
        [{"__name__": "temperature", "sensor": "1"}, [[1453395062, 28]]]
      ]
    }
  ]
}

```

#### Publish
Publish adds new values to the storage. Label 'src' is implicitly added to each timeseries.

Arguments:
- `timestamp`: Timestamp.
- `vars`: Each element must be an array with 2 elements: an object, which keys and values are used as label names and label values respectively, and a number - the actual value.


Definition:
```json
{
  "doc": "Publish adds new values to the storage. Label 'src' is implicitly added to each timeseries.",
  "args": {
    "timestamp": {
      "doc": "Timestamp.",
      "type": "integer"
    },
    "vars": {
      "doc": "Each element must be an array with 2 elements: an object, which keys and values are used as label names and label values respectively, and a number - the actual value.",
      "type": "array",
      "items": {
        "minItems": 2,
        "items": [
          {
            "additionalProperties": {
              "type": "string"
            },
            "doc": "Label set.",
            "required": [
              "__name__"
            ],
            "type": "object"
          },
          {
            "doc": "Value.",
            "type": "number"
          }
        ],
        "additionalItems": false,
        "type": "array"
      }
    }
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

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com",
  "dst": "//api.cesanta.com/device_123",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
}
```


