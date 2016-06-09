---
title: "Project"
---

Provides methods to manage projects.

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

#### Get
Returns project info for a given project

Arguments:
- `id`: ID of the project to be retreived

Result `object`: 
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

#### Delete
Deletes a project

Arguments:
- `id`: ID of the project to be deleted.


