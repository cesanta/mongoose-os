---
title: "User service"
---

Provides methods for managing users. Used by the frontend.

#### Create
Creates a new user. Can only be called by the frontend.

Arguments:
- `password`: Password for the user.
- `email`: User's email address.
- `name`: Display name for the user.
- `id`: ID for the new user.


Definition:
```json
{
  "doc": "Creates a new user. Can only be called by the frontend.",
  "args": {
    "password": {
      "doc": "Password for the user.",
      "type": "string"
    },
    "email": {
      "doc": "User's email address.",
      "type": "string"
    },
    "name": {
      "doc": "Display name for the user.",
      "type": "string"
    },
    "id": {
      "doc": "ID for the new user.",
      "type": "string"
    }
  },
  "required_args": [
    "id"
  ]
}
```

Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/cloud.frontend",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/User.Create",
      "id": 123,
      "args": {
        "id": "//api.cesanta.com/user_123",
        "password": "asdf",
        "email": "user123@example.com",
        "name": "John Doe"
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
  "dst": "//api.cesanta.com/cloud.frontend",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
}

```

#### Get
Retrieves info about an existing user.

Arguments:
- `id`: ID of the user.

Result `object`: 

Definition:
```json
{
  "doc": "Retrieves info about an existing user.",
  "args": {
    "id": {
      "doc": "ID of the user.",
      "type": "string"
    }
  },
  "required_args": [
    "id"
  ],
  "result": {
    "additionalProperties": false,
    "type": "object",
    "properties": {
      "name": {
        "type": "string"
      },
      "email": {
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
  "src": "//api.cesanta.com/cloud.frontend",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/User.Get",
      "id": 123,
      "args": {
        "id": "//api.cesanta.com/user_123"
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
  "dst": "//api.cesanta.com/cloud.frontend",
  "resp": [
    {
      "id": 123,
      "status": 0,
      "resp": {
        "name": "John Doe",
        "email": "user123@example.com"
      }
    }
  ]
}

```


