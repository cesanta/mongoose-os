---
title: "Blob"
---

Blob service provides basic key-value store. Keys are arrays of strings. First component of the key must be either your ID or ID of a project you have access to.

#### Get
Arguments:
- `key`: Key to fetch the value for.

Result `['object', 'array', 'string']`: Value stored with a given key.
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Blob.Get",
  "args": {
    "key": "VALUE PLACEHOLDER"
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

#### Set
Stores data at a given key. If binary flag is set, value must be an array of numbers in 0-255 range.

Arguments:
- `binary`: If set to `true`, `value` will be stored in a binary form (not JSON). Only works if the value is an array of numbers in 0-255 range, each number is interpreted as a byte value.
- `key`: Key to set value for.
- `value`: Value to store: object, array or a string.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Blob.Set",
  "args": {
    "binary": "VALUE PLACEHOLDER",
    "key": "VALUE PLACEHOLDER",
    "value": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
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
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Blob.List",
  "args": {
    "end": "VALUE PLACEHOLDER",
    "inclusive": "VALUE PLACEHOLDER",
    "limit": "VALUE PLACEHOLDER",
    "prefix": "VALUE PLACEHOLDER",
    "start": "VALUE PLACEHOLDER"
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

#### Delete
Deletes the given keys.

Arguments:
- `keys`: Keys to delete.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Blob.Delete",
  "args": {
    "keys": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```


