---
title: "SWUpdate service"
---

SWUpdate service provides a way to update device's software.

#### ListSections
Returns a list of components of the device's software. Each section is updated individually.


Result `array`: 

Definition:
```json
{
  "doc": "Returns a list of components of the device's software. Each section is updated individually.",
  "result": {
    "items": {
      "type": "object",
      "properties": {
        "writable": {
          "type": "boolean"
        },
        "section": {
          "type": "string"
        },
        "version": {
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
  "dst": "//api.cesanta.com/device_123",
  "cmds": [
    {
      "cmd": "/v1/SWUpdate.ListSections",
      "id": 123
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/device_123",
  "dst": "//api.cesanta.com/user_123",
  "resp": [
    {
      "id": 123,
      "status": 0,
      "resp": [
        {
          "section": "firmware",
          "version": "1234",
          "writable": true
        }
      ]
    }
  ]
}

```

#### Update
Instructs the device to update a given section.

Arguments:
- `section`: Name of the section to update.
- `version`: Optional version of the new image.
- `sig`: Hash or signature for the image that can be used to verify its integrity.
- `blob`: Image as a string, if appropriate.
- `blob_url`: URL pointing to the image if it's too big to fit in the `blob`.


Definition:
```json
{
  "doc": "Instructs the device to update a given section.",
  "args": {
    "section": {
      "doc": "Name of the section to update.",
      "type": "string"
    },
    "version": {
      "doc": "Optional version of the new image.",
      "type": "string"
    },
    "sig": {
      "doc": "Hash or signature for the image that can be used to verify its integrity.",
      "required": [
        "alg",
        "v"
      ],
      "type": "object",
      "properties": {
        "alg": {
          "type": "string"
        },
        "v": {
          "type": "string"
        }
      }
    },
    "blob": {
      "doc": "Image as a string, if appropriate.",
      "type": "string"
    },
    "blob_url": {
      "doc": "URL pointing to the image if it's too big to fit in the `blob`.",
      "type": "string"
    }
  }
}
```

Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/user_123",
  "dst": "//api.cesanta.com/device_123",
  "cmds": [
    {
      "cmd": "/v1/SWUpdate.Update",
      "id": 123,
      "args": {
        "blob_url": "https://my.server/fw/123/metadata.json",
        "section": "firmware",
        "version": "123"
      }
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/device_123",
  "dst": "//api.cesanta.com/user_123",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
}

```


