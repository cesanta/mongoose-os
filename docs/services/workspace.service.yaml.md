---
title: "Workspace"
---

A workspace is a place where the user stores her code. The Cloud IDE and Build services operate on workspaces. The actual content of the workspace is stored in the blobstore. The Workspace service provides operation for creating, listing and downloading workspaces, which are the core operations that an IDE an a build worker need.

#### Download
Pack the workspace (or a workspace subdirectory) in a zip and provide the raw bytes

Arguments:
- `path`: 

Result `['array', 'string']`: ZIP. As binary blob
#### List
List workspaces

Arguments:
- `filter`: 

Result `array`: 
#### Create
Create new workspace filled with a skeleton project.

Arguments:
- `projectid`: ID of the project
- `id`: Optional ID of the workspace. Must be unique within a project. Useful to create well known workspaces.
- `template`: optional skeleton template

Result `array`: Workspace path
#### ListTemplates
List templates. All templates visible to the given caller will be listed


Result `array`: 
#### Delete
Delete a workspace.

Arguments:
- `path`: 


