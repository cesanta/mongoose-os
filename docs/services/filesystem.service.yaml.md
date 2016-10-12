---
title: "Filesystem"
---

The FileSystem service exposes a filesystem-like interface for cloud storage. All relative paths are interpreted as path in the home directory of the caller.

#### Read
Reads a file.


Arguments:
- `path`: File path.
- `enc`: Result encoding. Valid values are:
  `none`: the data will be returned as a string, with best effort quoting
  of binary data as permitted by the JSON standard.
  `base64`: the data will be encoded with base64, as defined in RFC 4648.

  If omitted `none` is assumed.

- `len`: Length of chunk to read. If omitted, all available data until the EOF
will be read. If (offset + len) is larger than the file size, no
error will be returned, and only available data until the EOF will be
read.

- `offset`: Offset from the beginning of the file to start reading from.
If omitted, 0 is assumed. If the given offset is larger than the file
size, no error is returned, and the returned data will be null.


Result `object`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Filesystem.Read",
  "args": {
    "enc": "VALUE PLACEHOLDER",
    "len": "VALUE PLACEHOLDER",
    "offset": "VALUE PLACEHOLDER",
    "path": "VALUE PLACEHOLDER"
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

#### Write
Writes a file.


Arguments:
- `path`: File path.
- `enc`: Input encoding. Valid values are:
  `none`: the data will be parsed as a string, with best effort quoting
  of binary data as permitted by the JSON standard.
  `base64`: the data will be parsed as (padded) base64, as defined in RFC 4648.

  If omitted `none` is assumed.

- `data`: File contents.
- `append`: If true, and if the file with the given filename already exists, the
data will be appended to it. Otherwise, the file will be overwritten
or created.

- `create_intermediate_dirs`: Create intermediate directories as required. Default false.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Filesystem.Write",
  "args": {
    "append": "VALUE PLACEHOLDER",
    "create_intermediate_dirs": "VALUE PLACEHOLDER",
    "data": "VALUE PLACEHOLDER",
    "enc": "VALUE PLACEHOLDER",
    "path": "VALUE PLACEHOLDER"
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

#### Mkdir
Creates a directory.


Arguments:
- `path`: Directory path.
- `create_intermediate_dirs`: Create intermediate directories as required. Default false.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Filesystem.Mkdir",
  "args": {
    "create_intermediate_dirs": "VALUE PLACEHOLDER",
    "path": "VALUE PLACEHOLDER"
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

#### List
Lists a directory.


Arguments:
- `path`: Directory path.

Result `array`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Filesystem.List",
  "args": {
    "path": "VALUE PLACEHOLDER"
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
Deletes the given file or directory.


Arguments:
- `path`: File path.
- `recursive`: If path points to a directory, delete its contents recursively.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/Filesystem.Delete",
  "args": {
    "path": "VALUE PLACEHOLDER",
    "recursive": "VALUE PLACEHOLDER"
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


