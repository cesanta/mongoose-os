---
title: "Account"
---

Provides methods for managing users. Used by the frontend.

#### CreateToken
Generate a personal access token that can be passed in GET parameters instead user/psk pair

Arguments:
- `description`: displayed when listing the tokens

Result `string`: 
#### ListTokens
List tokens


Result `array`: 
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

#### RevokeToken
Delete the token

Arguments:
- `token`: 

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

#### ValidateToken
validates a token and returns the user ID associated with it

Arguments:
- `token`: 

Result `object`: 

