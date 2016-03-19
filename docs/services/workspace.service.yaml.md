---
title: "Workspace service"
---

A workspace is a place where the user stores her code. The Cloud IDE and Build services operate on workspaces. The actual content of the workspace is stored in the blobstore. The Workspace service provides operation for creating, listing and downloading workspaces, which are the core operations that an IDE an a build worker need.

#### Download
Pack the workspace in a zip and provide the raw bytes

Arguments:
- `path`: 

Result `['array', 'string']`: ZIP. As binary blob

Definition:
```json
{
  "doc": "Pack the workspace in a zip and provide the raw bytes",
  "args": {
    "path": {
      "items": {
        "type": "string"
      },
      "type": "array"
    }
  },
  "required_args": [
    "path"
  ],
  "result": {
    "doc": "ZIP. As binary blob",
    "type": [
      "array",
      "string"
    ],
    "keep_as_json": true
  }
}
```

#### List
List workspaces

Arguments:
- `filter`: 

Result `array`: 

Definition:
```json
{
  "doc": "List workspaces",
  "args": {
    "filter": {
      "type": "object",
      "properties": {
        "projectid": {
          "doc": "List only builds matching workspaces belonging to a given project. Regardless of this filter, the result will only contain workspaces visible to the caller.",
          "type": "string"
        }
      }
    }
  },
  "result": {
    "items": {
      "type": "object",
      "properties": {
        "path": {
          "items": {
            "type": "string"
          },
          "type": "array",
          "doc": "Workspace path"
        }
      }
    },
    "type": "array"
  }
}
```

#### Create
Create new workspace filled with a skeleton project.

Arguments:
- `projectid`: ID of the project
- `id`: Optional ID of the workspace. Must be unique within a project. Useful to create well known workspaces.
- `template`: optional skeleton template

Result `array`: Workspace path

Definition:
```json
{
  "doc": "Create new workspace filled with a skeleton project.",
  "args": {
    "projectid": {
      "doc": "ID of the project",
      "type": "string"
    },
    "id": {
      "doc": "Optional ID of the workspace. Must be unique within a project. Useful to create well known workspaces.",
      "type": "string"
    },
    "template": {
      "doc": "optional skeleton template",
      "type": "string"
    }
  },
  "required_args": [
    "projectid"
  ],
  "result": {
    "items": {
      "type": "string"
    },
    "type": "array",
    "doc": "Workspace path"
  }
}
```

#### ListTemplates
List templates. All templates visible to the given caller will be listed


Result `array`: 

Definition:
```json
{
  "doc": "List templates. All templates visible to the given caller will be listed",
  "result": {
    "items": {
      "type": "string"
    },
    "type": "array"
  }
}
```

#### Delete
Delete a workspace.

Arguments:
- `path`: 


Definition:
```json
{
  "doc": "Delete a workspace.",
  "args": {
    "path": {
      "items": {
        "type": "string"
      },
      "type": "array"
    }
  },
  "required_args": [
    "path"
  ]
}
```


