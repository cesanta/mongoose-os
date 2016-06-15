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
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Project.CheckAccess",
  "args": {
    "level": "VALUE PLACEHOLDER",
    "projectid": "VALUE PLACEHOLDER",
    "userid": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.cesanta.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```

#### Get
Returns project info for a given project

Arguments:
- `id`: ID of the project to be retreived

Result `object`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Project.Get",
  "args": {
    "id": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.cesanta.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
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
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Project.RevokeAccess",
  "args": {
    "projectid": "VALUE PLACEHOLDER",
    "userid": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.cesanta.com",
  "dst": "device_123",
  "id": 123
}

```

#### Create
Creates a new project.

Arguments:
- `owner`: ID of the owner for the new project. Set to ID of the caller by default.
- `id`: ID of a project. Assigned automatically if not specified.
- `name`: Unique name. The format is owner/project-name, only alphanumerical characters, dashes and underscores are allowed Will be prefixed with the owner account name. If the owner prefix is present, this method will check whether it matches the one specified via the owner argument.
- `summary`: Human-readable name for the project.

Result `string`: Project id.
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Project.Create",
  "args": {
    "id": "VALUE PLACEHOLDER",
    "name": "VALUE PLACEHOLDER",
    "owner": "VALUE PLACEHOLDER",
    "summary": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.cesanta.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
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
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Project.GrantAccess",
  "args": {
    "level": "VALUE PLACEHOLDER",
    "projectid": "VALUE PLACEHOLDER",
    "userid": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.cesanta.com",
  "dst": "device_123",
  "id": 123
}

```

#### List
Returns a list of projects the caller has access to.


Result `array`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Project.List",
  "args": {}
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.cesanta.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```

#### Delete
Deletes a project

Arguments:
- `id`: ID of the project to be deleted.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Project.Delete",
  "args": {
    "id": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.cesanta.com",
  "dst": "device_123",
  "id": 123
}

```


