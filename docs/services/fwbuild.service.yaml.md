---
title: "Build service"
---

The Build service allows to build a SJS project living in a Workspace.

#### List
List the last `limit` build jobs, filtered by an optional filter, ordered by descending acceptance time.

Arguments:
- `filter`: 
- `limit`: Maximum number of returned entries

Result `array`: 

Definition:
```json
{
  "doc": "List the last `limit` build jobs, filtered by an optional filter, ordered by descending acceptance time.",
  "args": {
    "filter": {
      "type": "object",
      "properties": {
        "projectid": {
          "doc": "list only builds matching workspaces belonging to a given project",
          "type": "string"
        },
        "id": {
          "doc": "list only the result which matches the given job ID",
          "type": "string"
        },
        "workspace": {
          "items": {
            "type": "string"
          },
          "type": "array",
          "doc": "list only builds matching a given workspace"
        }
      }
    },
    "limit": {
      "doc": "Maximum number of returned entries",
      "type": "integer"
    }
  },
  "result": {
    "items": {
      "type": "object",
      "properties": {
        "state": {
          "doc": "execution state: pending, running, done",
          "type": "string"
        },
        "outcome": {
          "doc": "once done, is it success or failure",
          "type": "string"
        },
        "startTime": {
          "doc": "timestamp of when the first task is switched to running state",
          "type": "string"
        },
        "artifacts": {
          "items": {
            "type": "object",
            "properties": {
              "path": {
                "items": {
                  "type": "string"
                },
                "type": "array"
              },
              "name": {
                "type": "string"
              }
            }
          },
          "type": "array",
          "doc": "Build artefacts such as FW zip files. Usually a Firmware build will produce only one artefact."
        },
        "submissionTime": {
          "doc": "timestamp of job submission",
          "type": "string"
        },
        "doneTime": {
          "doc": "timestamp of when the job is done",
          "type": "string"
        },
        "id": {
          "type": "string"
        }
      }
    },
    "type": "array"
  }
}
```

#### Build
Arguments:
- `workspace`: Path on the blobstore service to the root of the workspace to be built

Result `string`: build job ID

Definition:
```json
{
  "args": {
    "workspace": {
      "items": {
        "type": "string"
      },
      "type": "array",
      "doc": "Path on the blobstore service to the root of the workspace to be built"
    }
  },
  "required_args": [
    "workspace"
  ],
  "result": {
    "doc": "build job ID",
    "type": "string"
  }
}
```


