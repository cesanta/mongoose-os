---
title: "Build"
---

The Build service allows to build a SJS project living in a Workspace.

#### List
List the last `limit` build jobs, filtered by an optional filter, ordered by descending acceptance time.

Arguments:
- `filter`: 
- `limit`: Maximum number of returned entries

Result `array`: 
#### Build
Arguments:
- `workspace`: Path on the blobstore service to the root of the workspace to be built

Result `string`: build job ID

