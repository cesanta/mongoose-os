---
title: "Device"
---

Provides methods to manage devices.

#### Claim
Claim an unclaimed device using a token.

Arguments:
- `owner`: Name of the account that will own the device. Defaults to the identity of the caller
- `token`: Auth token based on PSK.
- `deviceid`: ID of the device.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Device.Claim",
  "args": {
    "deviceid": "VALUE PLACEHOLDER",
    "owner": "VALUE PLACEHOLDER",
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
  "id": 123
}

```

#### Add
Adds a new device.

Arguments:
- `owner`: Optional name of the account to add device to.
- `psk`: Pre-shared key that device will use for authentication.
- `force`: If set to `true`, then it won't be an error to call this method for
an already existing (and owned by the same user) device; instead, 
it will just update device psk.

- `deviceid`: ID of the device.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Device.Add",
  "args": {
    "deviceid": "VALUE PLACEHOLDER",
    "force": "VALUE PLACEHOLDER",
    "owner": "VALUE PLACEHOLDER",
    "psk": "VALUE PLACEHOLDER"
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

#### GetInfo
Gets device information.

Arguments:
- `deviceid`: ID of the device.

Result `object`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Device.GetInfo",
  "args": {
    "deviceid": "VALUE PLACEHOLDER"
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

#### List
Returns the list of owned devices

Arguments:
- `account`: name of account
- `labels`: An object with labels to query for. Object keys are label names, corresponding values are label values.

Result `array`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Device.List",
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

#### Delete
Unregisters the device

Arguments:
- `deviceid`: ID of the device.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Device.Delete",
  "args": {
    "deviceid": "VALUE PLACEHOLDER"
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


