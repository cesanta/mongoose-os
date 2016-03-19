---
title: "Project service"
---

Provides methods to manage projects.

#### ListDevicesWithMetadata
Deprecated method. Returns a list of devices in a project along with registration and last successful authentication timestamps.

Arguments:
- `projectid`: ID of the project.
- `extra_metadata`: List of extra fields that you want to get back. Accepted values are: "lastAuthTimestamp" and "registrationTimestamp".

Result `array`: 

Definition:
```json
{
  "doc": "Deprecated method. Returns a list of devices in a project along with registration and last successful authentication timestamps.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "extra_metadata": {
      "doc": "List of extra fields that you want to get back. Accepted values are: \"lastAuthTimestamp\" and \"registrationTimestamp\".",
      "type": "array",
      "items": {
        "enum": [
          "lastAuthTimestamp",
          "registrationTimestamp"
        ],
        "type": "string"
      }
    }
  },
  "required_args": [
    "projectid"
  ],
  "result": {
    "items": {
      "required": [
        "id"
      ],
      "type": "object",
      "properties": {
        "registrationTimestamp": {
          "type": "integer"
        },
        "lastAuthTimestamp": {
          "type": "integer"
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
      "cmd": "/v1/Project.ListDevicesWithMetadata",
      "id": 123,
      "args": {
        "projectid": "//api.cesanta.com/project_123",
        "extra_metadata": ["lastAuthTimestamp"]
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
          "lastAuthTimestamp": 1453395062
        }
      ]
    }
  ]
}

```

#### Create
Creates a new project.

Arguments:
- `owner`: ID of the owner for the new project. Set to ID of the caller by default.
- `id`: ID of a project. Assigned automatically if not specified.
- `name`: Human-readable name for the project.

Result `string`: Project id.

Definition:
```json
{
  "doc": "Creates a new project.",
  "args": {
    "owner": {
      "doc": "ID of the owner for the new project. Set to ID of the caller by default.",
      "type": "string"
    },
    "id": {
      "doc": "ID of a project. Assigned automatically if not specified.",
      "type": "string"
    },
    "name": {
      "doc": "Human-readable name for the project.",
      "type": "string"
    }
  },
  "result": {
    "doc": "Project id.",
    "type": "string"
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
      "cmd": "/v1/Project.Create",
      "id": 123,
      "args": {
        "id": "//api.cesanta.com/project_123",
        "name": "Default"
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

#### List
Returns a list of projects the caller has access to.


Result `array`: 

Definition:
```json
{
  "doc": "Returns a list of projects the caller has access to.",
  "result": {
    "items": {
      "type": "object",
      "properties": {
        "id": {
          "type": "string"
        },
        "name": {
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
      "cmd": "/v1/Project.List",
      "id": 123
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
          "id": "//api.cesanta.com/project_123",
          "name": "Default"
        }
      ]
    }
  ]
}

```

#### DeleteDevice
Removes the devices from the project.

Arguments:
- `projectid`: ID of the project.
- `deviceid`: ID of the device.


Definition:
```json
{
  "doc": "Removes the devices from the project.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "deviceid": {
      "doc": "ID of the device.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "deviceid"
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
      "cmd": "/v1/Project.DeleteDevice",
      "id": 123,
      "args": {
        "deviceid": "//api.cesanta.com/device_123",
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

#### ListDevices
Returns a list of devices in a given project.

Arguments:
- `filter`: Filter expression, currently only 'labelname=labelvalue' supported.
- `projectid`: ID of the project.

Result `array`: 

Definition:
```json
{
  "doc": "Returns a list of devices in a given project.",
  "args": {
    "filter": {
      "doc": "Filter expression, currently only 'labelname=labelvalue' supported.",
      "type": "string"
    },
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
      "type": "string"
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
      "cmd": "/v1/Project.ListDevices",
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
        "//api.cesanta.com/device_123"
      ]
    }
  ]
}

```

#### GrantAccess
Sets access level to the project for the user.

Arguments:
- `projectid`: ID of the project.
- `userid`: ID of the user.
- `level`: Access level. Currently defined levels are: 0 - no access, 10 - read access (e.g. can list devices, but not modify anything), 20 - write access (e.g. can add devices, upload firmware images, create rollouts), 30 - manage access (can grant and revoke privileges for other users).


Definition:
```json
{
  "doc": "Sets access level to the project for the user.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "userid": {
      "doc": "ID of the user.",
      "type": "string"
    },
    "level": {
      "doc": "Access level. Currently defined levels are: 0 - no access, 10 - read access (e.g. can list devices, but not modify anything), 20 - write access (e.g. can add devices, upload firmware images, create rollouts), 30 - manage access (can grant and revoke privileges for other users).",
      "type": "integer"
    }
  },
  "required_args": [
    "projectid",
    "userid",
    "level"
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
      "cmd": "/v1/Project.GrantAccess",
      "id": 123,
      "args": {
        "level": 20,
        "projectid": "//api.cesanta.com/project_123",
        "userid": "//api.cesanta.com/user_456"
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

#### RevokeAccess
Revokes access to the project for a given user.

Arguments:
- `projectid`: ID of the project.
- `userid`: ID of the user.


Definition:
```json
{
  "doc": "Revokes access to the project for a given user.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "userid": {
      "doc": "ID of the user.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "userid"
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
      "cmd": "/v1/Project.RevokeAccess",
      "id": 123,
      "args": {
        "projectid": "//api.cesanta.com/project_123",
        "userid": "//api.cesanta.com/user_456"
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

#### AddDevice
Adds a new device to the project.

Arguments:
- `projectid`: ID of the project.
- `deviceid`: ID of the device.
- `psk`: Pre-shared key that device will use for authentication.


Definition:
```json
{
  "doc": "Adds a new device to the project.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "deviceid": {
      "doc": "ID of the device.",
      "type": "string"
    },
    "psk": {
      "doc": "Pre-shared key that device will use for authentication.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "deviceid"
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
      "cmd": "/v1/Project.AddDevice",
      "id": 123,
      "args": {
        "projectid": "//api.cesanta.com/project_123",
        "deviceid": "//api.cesanta.com/device_123",
        "psk": "qwerty"
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

#### CheckAccess
Checks if a given user has a specified level of access to a given project.

Arguments:
- `projectid`: ID of the project.
- `userid`: ID of the user.
- `level`: Access level you want to confirm.

Result `boolean`: `true` if the user was granted a given access level, `false` otherwise.

Definition:
```json
{
  "doc": "Checks if a given user has a specified level of access to a given project.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "userid": {
      "doc": "ID of the user.",
      "type": "string"
    },
    "level": {
      "doc": "Access level you want to confirm.",
      "type": "integer"
    }
  },
  "required_args": [
    "projectid",
    "userid",
    "level"
  ],
  "result": {
    "doc": "`true` if the user was granted a given access level, `false` otherwise.",
    "type": "boolean"
  }
}
```

Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/cloud.blobstore",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/Project.CheckAccess",
      "id": 123,
      "args": {
        "projectid": "//api.cesanta.com/project_123",
        "userid": "//api.cesanta.com/user_123",
        "level": 10
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
  "dst": "//api.cesanta.com/cloud.blobstore",
  "resp": [
    {
      "id": 123,
      "status": 0,
      "resp": true
    }
  ]
}

```


