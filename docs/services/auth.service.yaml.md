---
title: "Auth service"
---

Auth service performs authentication and authorization of devices and commands. Can only be used by the cloud backend.

#### AuthorizeCommand
AuthorizeCommand returns true if `src` is allowed to send a given command to `dst`.

Arguments:
- `src`: ID of the command sender.
- `dst`: ID of the command recipient.
- `cmd`: Command name.

Result `boolean`: `true` if the command is allowed, `false` otherwise.

Definition:
```json
{
  "doc": "AuthorizeCommand returns true if `src` is allowed to send a given command to `dst`.",
  "args": {
    "src": {
      "doc": "ID of the command sender.",
      "type": "string"
    },
    "dst": {
      "doc": "ID of the command recipient.",
      "type": "string"
    },
    "cmd": {
      "doc": "Command name.",
      "type": "string"
    }
  },
  "required_args": [
    "src",
    "dst",
    "cmd"
  ],
  "result": {
    "doc": "`true` if the command is allowed, `false` otherwise.",
    "type": "boolean"
  }
}
```

Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/cloud.dispatcher",
  "dst": "//api.cesanta.com/cloud.registry",
  "cmds": [
    {
      "cmd": "/v1/Auth.AuthorizeCommand",
      "id": 123,
      "args": {
        "cmd": "/v1/Nuke.Launch",
        "dst": "//api.cesanta.com/misslecontrol",
        "src": "//api.cesanta.com/device_123"
      }
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/cloud.registry",
  "dst": "//api.cesanta.com/cloud.dispatcher",
  "resp": [
    {
      "id": 123,
      "status": 0,
      "resp": false
    }
  ]
}

```

#### Authenticate
Authenticate returns true if `id` is verified to have valid credentials.

Arguments:
- `credentials`: Credentials presented by the entity. At least one of `psk` or `cert` must be present.
- `id`: ID of an entity to authenticate.

Result `boolean`: `true` if credentials are valid, `false` otherwise.

Definition:
```json
{
  "doc": "Authenticate returns true if `id` is verified to have valid credentials.",
  "args": {
    "credentials": {
      "doc": "Credentials presented by the entity. At least one of `psk` or `cert` must be present.",
      "type": "object",
      "properties": {
        "psk": {
          "type": "string"
        },
        "cert": {
          "type": "string"
        }
      }
    },
    "id": {
      "doc": "ID of an entity to authenticate.",
      "type": "string"
    }
  },
  "required_args": [
    "id",
    "credentials"
  ],
  "result": {
    "doc": "`true` if credentials are valid, `false` otherwise.",
    "type": "boolean"
  }
}
```

Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/cloud.dispatcher",
  "dst": "//api.cesanta.com/cloud.registry",
  "cmds": [
    {
      "cmd": "/v1/Auth.Authenticate",
      "id": 123,
      "args": {
        "id": "//api.cesanta.com/device_123",
        "credentials": {
          "psk": "qwerty"
        }
      }
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/cloud.registry",
  "dst": "//api.cesanta.com/cloud.dispatcher",
  "resp": [
    {
      "id": 123,
      "status": 0,
      "resp": true
    }
  ]
}

```


