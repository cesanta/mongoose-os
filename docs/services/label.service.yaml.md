---
title: "Label"
---

Allows to manage arbitrary string labels for entities.

#### Get
Returns labels set on a particular entities.

Arguments:
- `labels`: Optional list of labels to fetch. If not set, all labels will be returned.
- `ids`: List of entity IDs to fetch labels for.

Result `array`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Label.Get",
  "args": {
    "ids": "VALUE PLACEHOLDER",
    "labels": "VALUE PLACEHOLDER"
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
Sets labels for entities.

Arguments:
- `labels`: An object with labels to set. Object keys are label names, corresponding values are label values to set.
- `ids`: List of entity IDs to set labels for.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Label.Set",
  "args": {
    "ids": "VALUE PLACEHOLDER",
    "labels": "VALUE PLACEHOLDER"
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

#### Delete
Deletes labels for entities.

Arguments:
- `labels`: List of names of labels to delete.
- `ids`: List of entity IDs to delete labels for.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Label.Delete",
  "args": {
    "ids": "VALUE PLACEHOLDER",
    "labels": "VALUE PLACEHOLDER"
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


