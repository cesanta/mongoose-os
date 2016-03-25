---
title: "Frontend service"
---

Misc APIs provided by the HTTP frontend.

#### BlobURL
Returns an HTTP URL which points to the given blobstore key.

Arguments:
- `key`: Blobstore key.

Result `string`: A URL.

Definition:
```json
{
  "doc": "Returns an HTTP URL which points to the given blobstore key.",
  "args": {
    "key": {
      "minItems": 1,
      "doc": "Blobstore key.",
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
    "doc": "A URL.",
    "type": "string"
  }
}
```


