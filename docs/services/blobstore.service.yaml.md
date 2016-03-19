---
title: "Blob service"
---

Blob service provides basic key-value store. Keys are arrays of strings. First component of the key must be either your ID or ID of a project you have access to.

#### Get
Arguments:
- `key`: Key to fetch the value for.

Result `['object', 'array', 'string']`: Value stored with a given key.

Definition:
```json
{
  "args": {
    "key": {
      "minItems": 1,
      "doc": "Key to fetch the value for.",
      "type": "array",
      "items": {
        "type": "string"
      }
    }
  },
  "required_args": [
    "key"
  ],
  "result": {
    "doc": "Value stored with a given key.",
    "type": [
      "object",
      "array",
      "string"
    ],
    "keep_as_json": true
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
      "cmd": "/v1/Blob.Get",
      "id": 123,
      "args": {
        "key": ["//api.cesanta.com/device_123", "some", "stuff"]
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
      "status": 0,
      "resp": {
        "last_file": "/etc/passwd"
      }
    }
  ]
}

```

#### Set
Stores data at a given key. If binary flag is set, value must be an array of numbers in 0-255 range.

Arguments:
- `binary`: If set to `true`, `value` will be stored in a binary form (not JSON). Only works if the value is an array of numbers in 0-255 range, each number is interpreted as a byte value.
- `key`: Key to set value for.
- `value`: Value to store: object, array or a string.


Definition:
```json
{
  "doc": "Stores data at a given key. If binary flag is set, value must be an array of numbers in 0-255 range.",
  "args": {
    "binary": {
      "doc": "If set to `true`, `value` will be stored in a binary form (not JSON). Only works if the value is an array of numbers in 0-255 range, each number is interpreted as a byte value.",
      "type": "boolean"
    },
    "key": {
      "minItems": 1,
      "doc": "Key to set value for.",
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "value": {
      "doc": "Value to store: object, array or a string.",
      "type": [
        "object",
        "array",
        "string"
      ],
      "keep_as_json": true
    }
  },
  "required_args": [
    "key",
    "value"
  ]
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
      "cmd": "/v1/Blob.Set",
      "id": 123,
      "args": {
        "key": ["//api.cesanta.com/device_123", "some", "stuff"],
        "value": {
          "last_file": "/etc/passwd"
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
  "dst": "//api.cesanta.com/device_123",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
}
```

#### List
Returns a list of all keys with a given prefix. Within that prefix, items can be iterated from a specific start key (inclusive), up to an optional end key (non inclusive). If inclusive is false (true by default), then the start key is non inclusive. The result can be limited to a given number of items with the limit parameter.

Arguments:
- `start`: If set, only keys that compare after or equal to this key will be returned.
- `prefix`: Key prefix (few first components) to list keys under.
- `end`: If set, only keys that compare before this key will be returned.
- `limit`: Maximum number of entries to return.
- `inclusive`: Set this to `false` to omit key equal to `start` in the result.

Result `array`: List of matched keys currently present in the blobstore.

Definition:
```json
{
  "doc": "Returns a list of all keys with a given prefix. Within that prefix, items can be iterated from a specific start key (inclusive), up to an optional end key (non inclusive). If inclusive is false (true by default), then the start key is non inclusive. The result can be limited to a given number of items with the limit parameter.",
  "args": {
    "start": {
      "minItems": 1,
      "doc": "If set, only keys that compare after or equal to this key will be returned.",
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "prefix": {
      "minItems": 1,
      "doc": "Key prefix (few first components) to list keys under.",
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "end": {
      "minItems": 1,
      "doc": "If set, only keys that compare before this key will be returned.",
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "limit": {
      "doc": "Maximum number of entries to return.",
      "type": "integer"
    },
    "inclusive": {
      "doc": "Set this to `false` to omit key equal to `start` in the result.",
      "type": "boolean"
    }
  },
  "result": {
    "doc": "List of matched keys currently present in the blobstore.",
    "type": "array",
    "items": {
      "minItems": 1,
      "items": {
        "type": "string"
      },
      "type": "array"
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
      "cmd": "/v1/Blob.List",
      "id": 123,
      "args": {
        "prefix": ["//api.cesanta.com/device_123", "some"],
        "limit": 10
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
      "status": 0,
      "resp": [
        ["//api.cesanta.com/device_123", "some", "stuff"]
      ]
    }
  ]
}

```

#### Delete
Deletes the given keys.

Arguments:
- `keys`: Keys to delete.


Definition:
```json
{
  "doc": "Deletes the given keys.",
  "args": {
    "keys": {
      "doc": "Keys to delete.",
      "type": "array",
      "items": {
        "minItems": 1,
        "items": {
          "type": "string"
        },
        "type": "array"
      }
    }
  },
  "required_args": [
    "keys"
  ]
}
```


