---
title: "Firmware service"
---

Firmware service provides means for managing a set of firmware images.

#### Create
Registers a new firmware image.

Arguments:
- `projectid`: ID of the project to register firmware for.
- `version`: Firmware version number. Encoded as a string due to limitations of JavaScript.
- `name`: Human-readable name for this firmware.
- `zip`: URL of the .zip archive suitable for flashing.
- `manifest`: URL of the firmware metadata file. Must be accessible by the devices that will get this firmware.


Definition:
```json
{
  "doc": "Registers a new firmware image.",
  "args": {
    "projectid": {
      "doc": "ID of the project to register firmware for.",
      "type": "string"
    },
    "version": {
      "doc": "Firmware version number. Encoded as a string due to limitations of JavaScript.",
      "type": "string"
    },
    "name": {
      "doc": "Human-readable name for this firmware.",
      "type": "string"
    },
    "zip": {
      "doc": "URL of the .zip archive suitable for flashing.",
      "type": "string"
    },
    "manifest": {
      "doc": "URL of the firmware metadata file. Must be accessible by the devices that will get this firmware.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "manifest",
    "version"
  ]
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
      "cmd": "/v1/Firmware.Create",
      "id": 123,
      "args": {
        "manifest": "https://my.server/fw/123/metadata.json",
        "name": "New firmware",
        "projectid": "//api.cesanta.com/project_123",
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

#### Get
Returns information about a given firmware.

Arguments:
- `projectid`: ID of the project.
- `id`: ID of the firmware image. IDs are assigned by the backend, use `List` command to get the it initially.

Result `object`: 

Definition:
```json
{
  "doc": "Returns information about a given firmware.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "id": {
      "doc": "ID of the firmware image. IDs are assigned by the backend, use `List` command to get the it initially.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "id"
  ],
  "result": {
    "type": "object",
    "properties": {
      "version": {
        "type": "string"
      },
      "name": {
        "type": "string"
      },
      "zip": {
        "type": "string"
      },
      "created": {
        "type": "string"
      },
      "id": {
        "type": "string"
      },
      "manifest": {
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
      "cmd": "/v1/Firmware.Get",
      "id": 123,
      "args": {
        "id": "1234567890",
        "projectid": "//api.cesanta.com/project_123"
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
      "resp": {
        "id": "1234567980",
        "name": "New firmware",
        "manifest": "https://my.server/fw/123/metadata.json",
        "version": "123",
        "created": "1453395062"
      }
    }
  ]
}

```

#### List
Returns a list of firmware images registered in a given project.

Arguments:
- `projectid`: ID of the project.

Result `array`: 

Definition:
```json
{
  "doc": "Returns a list of firmware images registered in a given project.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid"
  ],
  "result": {
    "items": {
      "type": "object",
      "properties": {
        "version": {
          "type": "string"
        },
        "name": {
          "type": "string"
        },
        "zip": {
          "type": "string"
        },
        "created": {
          "type": "string"
        },
        "id": {
          "type": "string"
        },
        "manifest": {
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
      "cmd": "/v1/Firmware.List",
      "id": 123,
      "args": {
        "projectid": "//api.cesanta.com/project_123"
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
          "id": "1234567980",
          "name": "New firmware",
          "manifest": "https://my.server/fw/123/metadata.json",
          "version": "123",
          "created": "1453395062"
        }
      ]
    }
  ]
}

```

#### Update
Allows to change the name of the firmware image.

Arguments:
- `projectid`: ID of the project.
- `id`: ID of the firmware image.
- `name`: New name for the firmware image.


Definition:
```json
{
  "doc": "Allows to change the name of the firmware image.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "id": {
      "doc": "ID of the firmware image.",
      "type": "string"
    },
    "name": {
      "doc": "New name for the firmware image.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "id"
  ]
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
      "cmd": "/v1/Firmware.Update",
      "id": 123,
      "args": {
        "id": "1234567890",
        "name": "Old firmware",
        "projectid": "//api.cesanta.com/project_123"
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
Deletes the firmware image from the database.

Arguments:
- `projectid`: ID of the project.
- `id`: ID of the firmware image.


Definition:
```json
{
  "doc": "Deletes the firmware image from the database.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "id": {
      "doc": "ID of the firmware image.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "id"
  ]
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
      "cmd": "/v1/Firmware.Delete",
      "id": 123,
      "args": {
        "id": "1234567890",
        "projectid": "//api.cesanta.com/project_123"
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


