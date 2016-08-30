---
title: "Account"
---

Provides methods for managing users. Used by the frontend.

#### GetMembership
Set group membership info.


Arguments:
- `account`: Account ID.
- `group`: Group ID.

Result `array`: List of role names.
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.GetMembership",
  "args": {
    "account": "VALUE PLACEHOLDER",
    "group": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```

#### ListGroups
List groups.


Arguments:
- `account`: Optional account name or ID. If omitted sender is implied.
- `labels`: An object with labels to query for. Object keys are label names, corresponding values are label values.

Result `array`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.ListGroups",
  "args": {
    "account": "VALUE PLACEHOLDER",
    "labels": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```

#### ListTokens
List tokens


Result `array`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.ListTokens",
  "args": {}
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```

#### GetInfo
Retrieves info about an existing user. If id is present, info is fetched by id. Otherwise, if name is present, info is fetched by name. Otherwise, it's an error.

Arguments:
- `id`: ID of the user.
- `name`: Name of the user.

Result `object`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.GetInfo",
  "args": {
    "id": "VALUE PLACEHOLDER",
    "name": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```

#### RevokeToken
Delete the token

Arguments:
- `id`: 

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.RevokeToken",
  "args": {
    "id": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```

#### UserExists
Returns whether the user exists

Arguments:
- `id`: ID of the user.
- `name`: Name of the user.

Result `object`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.UserExists",
  "args": {
    "id": "VALUE PLACEHOLDER",
    "name": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```

#### DeleteGroup
Deletes a group.


Arguments:
- `id`: ID for the group to be deleted.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.DeleteGroup",
  "args": {
    "id": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```

#### CreateToken
Generate a personal access token that can be passed in GET parameters instead user/psk pair

Arguments:
- `description`: displayed when listing the tokens

Result `string`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.CreateToken",
  "args": {
    "description": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```

#### SetMembership
Set group membership.
To remove an account from a group pass an empty roles list


Arguments:
- `account`: Account ID.
- `group`: Group ID.
- `roles`: List of role names.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.SetMembership",
  "args": {
    "account": "VALUE PLACEHOLDER",
    "group": "VALUE PLACEHOLDER",
    "roles": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```

#### CreateGroup
Creates a new group.


Arguments:
- `labels`: An object with labels to set. Object keys are label names, corresponding values are label values to set.
- `id`: ID for the new group.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.CreateGroup",
  "args": {
    "id": "VALUE PLACEHOLDER",
    "labels": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```

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
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```

#### ValidateToken
validates a token and returns the user ID associated with it

Arguments:
- `token`: 

Result `object`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Account.ValidateToken",
  "args": {
    "token": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```


