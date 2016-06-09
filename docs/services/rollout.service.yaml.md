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


