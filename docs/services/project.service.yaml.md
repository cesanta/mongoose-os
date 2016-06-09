---
title: "Project"
---

Provides methods to manage projects.

#### GetDevice
Gets device information.

Arguments:
- `deviceid`: ID of the device.

Result `object`: 
#### ListDevicesWithMetadata
Deprecated method. Returns a list of devices in a project along with registration and last successful authentication timestamps.

Arguments:
- `projectid`: ID of the project.
- `extra_metadata`: List of extra fields that you want to get back. Accepted values are: "lastAuthTimestamp" and "registrationTimestamp".

Result `array`: 
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

#### Get
Returns project info for a given project

Arguments:
- `id`: ID of the project to be retreived

Result `object`: 
#### Create
Creates a new project.

Arguments:
- `owner`: ID of the owner for the new project. Set to ID of the caller by default.
- `id`: ID of a project. Assigned automatically if not specified.
- `name`: Human-readable name for the project.

Result `string`: Project id.
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

#### ClaimDevice
Claim an unclaimed device using a token.

Arguments:
- `projectid`: ID of the project.
- `token`: Auth token based on PSK.
- `deviceid`: ID of the device.

#### ListDevices
Returns a list of devices in a given project.

Arguments:
- `filter`: Filter expression, currently only 'labelname=labelvalue' supported.
- `projectid`: ID of the project.

Result `array`: 
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
Adds a new device.

Arguments:
- `psk`: Pre-shared key that device will use for authentication.
- `deviceid`: ID of the device.
- `projectid`: Optional ID of the project to add device to.

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

#### Delete
Deletes a project

Arguments:
- `id`: ID of the project to be deleted.

#### CheckAccess
Checks if a given user has a specified level of access to a given project.

Arguments:
- `projectid`: ID of the project.
- `userid`: ID of the user.
- `level`: Access level you want to confirm.

Result `boolean`: `true` if the user was granted a given access level, `false` otherwise.
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


