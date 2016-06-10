---
title: "Account"
---

Provides methods for managing users. Used by the frontend.

#### CreateUser
Creates a new user. Can only be called by the frontend.

Arguments:
- `password`: Password for the user.
- `email`: User's email address.
- `name`: Display name for the user.
- `id`: ID for the new user.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.CreateUser",
  "args": {
    "email": "VALUE PLACEHOLDER",
    "id": "VALUE PLACEHOLDER",
    "name": "VALUE PLACEHOLDER",
    "password": "VALUE PLACEHOLDER"
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

#### GetInfo
Retrieves info about an existing user.

Arguments:
- `id`: ID of the user.

Result `object`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.GetInfo",
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


