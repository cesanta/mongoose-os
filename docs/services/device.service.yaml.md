---
title: "Device"
---

Provides methods to manage devices.

#### Claim
Claim an unclaimed device using a token.

Arguments:
- `projectid`: ID of the project.
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
    "projectid": "VALUE PLACEHOLDER",
    "token": "VALUE PLACEHOLDER"
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

#### Add
Adds a new device.

Arguments:
- `psk`: Pre-shared key that device will use for authentication.
- `deviceid`: ID of the device.
- `projectid`: Optional ID of the project to add device to.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Device.Add",
  "args": {
    "deviceid": "VALUE PLACEHOLDER",
    "projectid": "VALUE PLACEHOLDER",
    "psk": "VALUE PLACEHOLDER"
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
  "src": "//api.cesanta.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```

#### List
Returns a list of devices in a given project.

Arguments:
- `filter`: Filter expression, currently only 'labelname=labelvalue' supported.
- `projectid`: ID of the project.

Result `array`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Device.List",
  "args": {
    "filter": "VALUE PLACEHOLDER",
    "projectid": "VALUE PLACEHOLDER"
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

#### Delete
Removes the devices from the project.

Arguments:
- `projectid`: ID of the project.
- `deviceid`: ID of the device.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Device.Delete",
  "args": {
    "deviceid": "VALUE PLACEHOLDER",
    "projectid": "VALUE PLACEHOLDER"
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


