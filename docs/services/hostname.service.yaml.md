---
title: "Hostname service"
---

Hostname service allows to control mapping between hostnames and blobstore key prefixes used as web-server root directories.

#### Add
Adds new hostname to the database.

Arguments:
- `projectid`: ID of the project this hostname belongs to. Caller must have access to this project.
- `hostname`: Hostname.
- `root`: Blobstore key prefix. First component must be the same as `projectid`.


Definition:
```json
{
  "doc": "Adds new hostname to the database.",
  "args": {
    "projectid": {
      "doc": "ID of the project this hostname belongs to. Caller must have access to this project.",
      "type": "string"
    },
    "hostname": {
      "doc": "Hostname.",
      "type": "string"
    },
    "root": {
      "items": {
        "type": "string"
      },
      "type": "array",
      "doc": "Blobstore key prefix. First component must be the same as `projectid`."
    }
  },
  "required_args": [
    "hostname",
    "projectid",
    "root"
  ]
}
```

#### Delete
Deletes the hostname from the database.

Arguments:
- `hostname`: Hostname.


Definition:
```json
{
  "doc": "Deletes the hostname from the database.",
  "args": {
    "hostname": {
      "doc": "Hostname.",
      "type": "string"
    }
  },
  "required_args": [
    "hostname"
  ]
}
```

#### Update
Changes the root blobstore prefix.

Arguments:
- `hostname`: Hostname.
- `root`: New blobstore key prefix. First component must match the ID of the project this hostname belongs to.


Definition:
```json
{
  "doc": "Changes the root blobstore prefix.",
  "args": {
    "hostname": {
      "doc": "Hostname.",
      "type": "string"
    },
    "root": {
      "items": {
        "type": "string"
      },
      "type": "array",
      "doc": "New blobstore key prefix. First component must match the ID of the project this hostname belongs to."
    }
  },
  "required_args": [
    "hostname",
    "root"
  ]
}
```

#### Get
Returns info about the hostname.

Arguments:
- `hostname`: Hostname.

Result `object`: An object containing info about the requested hostname.

Definition:
```json
{
  "doc": "Returns info about the hostname.",
  "args": {
    "hostname": {
      "doc": "Hostname.",
      "type": "string"
    }
  },
  "required_args": [
    "hostname"
  ],
  "result": {
    "doc": "An object containing info about the requested hostname.",
    "type": "object",
    "properties": {
      "projectid": {
        "type": "string"
      },
      "root": {
        "items": {
          "type": "string"
        },
        "type": "array"
      }
    }
  }
}
```


