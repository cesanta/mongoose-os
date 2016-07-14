---
title: "Auth"
---

Auth service performs authentication and authorization of devices and commands. Can only be used by the cloud backend.

#### GenerateToken
Returns auth token for the given app.

Arguments:
- `host`: Hostname of the app.

Result `object`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Auth.GenerateToken",
  "args": {
    "host": "VALUE PLACEHOLDER"
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

#### Authenticate
Authenticate returns true if `id` is verified to have valid credentials.

Arguments:
- `credentials`: Credentials presented by the entity. At least one of `psk` or `cert` must be present.
- `id`: ID of an entity to authenticate or a username.

Result `object`: `true` if credentials are valid, `false` otherwise.
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Auth.Authenticate",
  "args": {
    "credentials": "VALUE PLACEHOLDER",
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
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```

#### ListToken
Returns the list of apps for which tokens were generated.


Result `array`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Auth.ListToken",
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

#### AuthorizeCommand
AuthorizeCommand returns true if `src` is allowed to send a given command to `dst`.

Arguments:
- `src`: ID of the command sender.
- `dst`: ID of the command recipient.
- `cmd`: Command name.

Result `boolean`: `true` if the command is allowed, `false` otherwise.
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Auth.AuthorizeCommand",
  "args": {
    "cmd": "VALUE PLACEHOLDER",
    "dst": "VALUE PLACEHOLDER",
    "src": "VALUE PLACEHOLDER"
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
Revokes auth token for the given app.

Arguments:
- `host`: Hostname of the app.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Auth.RevokeToken",
  "args": {
    "host": "VALUE PLACEHOLDER"
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

#### GetToken
Returns auth token for the given app.

Arguments:
- `host`: Hostname of the app.

Result `object`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Auth.GetToken",
  "args": {
    "host": "VALUE PLACEHOLDER"
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


