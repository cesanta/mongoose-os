---
title: "Rollout service"
---

Rollout service stores info about firmware rollouts.

#### Get
Returns info about a particular rollout.

Arguments:
- `projectid`: ID of the project.
- `rolloutid`: ID of the rollout.

Result `object`: 

Definition:
```json
{
  "doc": "Returns info about a particular rollout.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "rolloutid": {
      "doc": "ID of the rollout.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "rolloutid"
  ],
  "result": {
    "type": "object",
    "properties": {
      "deviceFilter": {
        "type": "string"
      },
      "state": {
        "enum": [
          "init",
          "inProgress",
          "paused",
          "finished"
        ],
        "type": "string"
      },
      "id": {
        "type": "string"
      },
      "firmwareid": {
        "type": "string"
      },
      "name": {
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
      "cmd": "/v1/Rollout.Get",
      "id": 123,
      "args": {
        "projectid": "//api.cesanta.com/project_123",
        "rolloutid": "123"
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
        "id": "123",
        "name": "Today's rollout",
        "state": "inProgress",
        "deviceFilter": "location=Bathroom",
        "firmwareid": "1234567980"
      }
    }
  ]
}

```

#### Create
Creates new rollout.

Arguments:
- `deviceFilter`: Filter expression, currently only 'labelname=labelvalue' supported.
- `projectid`: ID of the project.
- `name`: Human-readable name for the rollout.
- `firmwareid`: ID of the firmware image.


Definition:
```json
{
  "doc": "Creates new rollout.",
  "args": {
    "deviceFilter": {
      "doc": "Filter expression, currently only 'labelname=labelvalue' supported.",
      "type": "string"
    },
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "name": {
      "doc": "Human-readable name for the rollout.",
      "type": "string"
    },
    "firmwareid": {
      "doc": "ID of the firmware image.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "firmwareid",
    "deviceFilter"
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
      "cmd": "/v1/Rollout.Create",
      "id": 123,
      "args": {
        "deviceFilter": "location=Bathroom",
        "firmwareid": "1234567980",
        "name": "Today's rollout",
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

#### List
Returns info about rollouts. If `projectid` and/or `state` are specified, only matching rollouts will be returned.

Arguments:
- `projectid`: ID of the project.
- `state`: If present, only rollouts in specified state will be returned.

Result `array`: List of objects describing matching rollouts.

Definition:
```json
{
  "doc": "Returns info about rollouts. If `projectid` and/or `state` are specified, only matching rollouts will be returned.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "state": {
      "doc": "If present, only rollouts in specified state will be returned.",
      "type": "string"
    }
  },
  "result": {
    "doc": "List of objects describing matching rollouts.",
    "type": "array",
    "items": {
      "type": "object",
      "properties": {
        "deviceFilter": {
          "type": "string"
        },
        "state": {
          "enum": [
            "init",
            "inProgress",
            "paused",
            "finished"
          ],
          "type": "string"
        },
        "name": {
          "type": "string"
        },
        "projectid": {
          "type": "string"
        },
        "id": {
          "type": "string"
        },
        "firmwareid": {
          "type": "string"
        }
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
      "cmd": "/v1/Rollout.List",
      "id": 123,
      "args": {
        "projectid": "//api.cesanta.com/project_123",
        "state": "inProgress"
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
          "id": "123",
          "name": "Today's rollout",
          "state": "inProgress",
          "deviceFilter": "location=Bathroom",
          "firmwareid": "1234567980"
        }
      ]
    }
  ]
}

```

#### AddDevices
Adds devices to the rollout. Mostly used by the backend.

Arguments:
- `projectid`: ID of the project.
- `devices`: List of the device IDs.
- `rolloutid`: ID of the rollout.


Definition:
```json
{
  "doc": "Adds devices to the rollout. Mostly used by the backend.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "devices": {
      "doc": "List of the device IDs.",
      "type": "array",
      "items": {
        "type": "string"
      }
    },
    "rolloutid": {
      "doc": "ID of the rollout.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "rolloutid",
    "devices"
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
      "cmd": "/v1/Rollout.AddDevices",
      "id": 123,
      "args": {
        "projectid": "//api.cesanta.com/project_123",
        "rolloutid": "123",
        "devices": ["//api.cesanta.com/device_123"]
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

#### SetState
Changes state of the rollout.

Arguments:
- `projectid`: ID of the project.
- `state`: Target state.
- `rolloutid`: ID of the rollout.


Definition:
```json
{
  "doc": "Changes state of the rollout.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "state": {
      "doc": "Target state.",
      "enum": [
        "init",
        "inProgress",
        "paused",
        "finished"
      ],
      "type": "string"
    },
    "rolloutid": {
      "doc": "ID of the rollout.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "rolloutid",
    "state"
  ]
}
```

Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/cloud.updater",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/Rollout.SetState",
      "id": 123,
      "args": {
        "projectid": "//api.cesanta.com/project_123",
        "rolloutid": "123",
        "state": "inProgress"
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
  "dst": "//api.cesanta.com/cloud.updater",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
}

```

#### ListDevices
Returns the list of devices previously added to the rollout.

Arguments:
- `projectid`: ID of the project.
- `rolloutid`: ID of the rollout.

Result `array`: 

Definition:
```json
{
  "doc": "Returns the list of devices previously added to the rollout.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "rolloutid": {
      "doc": "ID of the rollout.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "rolloutid"
  ],
  "result": {
    "items": {
      "type": "object",
      "properties": {
        "state": {
          "enum": [
            "init",
            "started",
            "succeeded",
            "failed"
          ],
          "type": "string"
        },
        "id": {
          "type": "string"
        },
        "updateCommandId": {
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
      "cmd": "/v1/Rollout.ListDevices",
      "id": 123,
      "args": {
        "projectid": "//api.cesanta.com/project_123",
        "rolloutid": "123"
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
          "state": "started",
          "updateCommandId": "12346878"
        }
      ]
    }
  ]
}

```

#### SetDeviceState
Changes the state of the device in rollout. Used by the backend.

Arguments:
- `projectid`: ID of the project.
- `state`: Target state.
- `deviceid`: ID of the device.
- `updateCommandId`: 
- `rolloutid`: ID of the rollout.


Definition:
```json
{
  "doc": "Changes the state of the device in rollout. Used by the backend.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "state": {
      "doc": "Target state.",
      "enum": [
        "init",
        "started",
        "succeeded",
        "failed"
      ],
      "type": "string"
    },
    "deviceid": {
      "doc": "ID of the device.",
      "type": "string"
    },
    "updateCommandId": "string",
    "rolloutid": {
      "doc": "ID of the rollout.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "rolloutid",
    "deviceid",
    "state"
  ]
}
```

Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/cloud.updater",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/Rollout.SetDeviceState",
      "id": 123,
      "args": {
        "deviceid": "//api.cesanta.com/device_123",
        "projectid": "//api.cesanta.com/project_123",
        "rolloutid": "123",
        "state": "succeeded"
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
  "dst": "//api.cesanta.com/cloud.updater",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
}

```

#### Delete
Deletes the rollout.

Arguments:
- `projectid`: ID of the project.
- `rolloutid`: ID of the rollout.


Definition:
```json
{
  "doc": "Deletes the rollout.",
  "args": {
    "projectid": {
      "doc": "ID of the project.",
      "type": "string"
    },
    "rolloutid": {
      "doc": "ID of the rollout.",
      "type": "string"
    }
  },
  "required_args": [
    "projectid",
    "rolloutid"
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
      "cmd": "/v1/Rollout.Delete",
      "id": 123,
      "args": {
        "projectid": "//api.cesanta.com/project_123",
        "rolloutid": "123"
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


