---
title: "AuthToken service"
---

AuthToken service allows to manipulate per-user auth tokens given out to 3rd-party apps.

#### Revoke
Revokes auth token for the given app.

Arguments:
- `host`: Hostname of the app.


Definition:
```json
{
  "doc": "Revokes auth token for the given app.",
  "args": {
    "host": {
      "doc": "Hostname of the app.",
      "type": "string"
    }
  },
  "required_args": [
    "host"
  ]
}
```

#### List
Returns the list of apps for which tokens were generated.


Result `array`: 

Definition:
```json
{
  "doc": "Returns the list of apps for which tokens were generated.",
  "result": {
    "items": {
      "type": "object",
      "properties": {
        "host": {
          "type": "string"
        }
      }
    },
    "type": "array"
  }
}
```

#### Generate
Returns auth token for the given app.

Arguments:
- `host`: Hostname of the app.

Result `object`: 

Definition:
```json
{
  "doc": "Returns auth token for the given app.",
  "args": {
    "host": {
      "doc": "Hostname of the app.",
      "type": "string"
    }
  },
  "required_args": [
    "host"
  ],
  "result": {
    "type": "object",
    "properties": {
      "token": {
        "type": "string"
      }
    }
  }
}
```

#### Get
Returns auth token for the given app.

Arguments:
- `host`: Hostname of the app.

Result `object`: 

Definition:
```json
{
  "doc": "Returns auth token for the given app.",
  "args": {
    "host": {
      "doc": "Hostname of the app.",
      "type": "string"
    }
  },
  "required_args": [
    "host"
  ],
  "result": {
    "type": "object",
    "properties": {
      "token": {
        "doc": "Token value. Not present if `exist` is `false`.",
        "type": "string"
      },
      "exist": {
        "doc": "`true` if token for the app was generated previously.",
        "type": "boolean"
      }
    }
  }
}
```


