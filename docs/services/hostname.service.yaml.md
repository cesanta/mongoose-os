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

#### Delete
Deletes the hostname from the database.

Arguments:
- `hostname`: Hostname.

#### Update
Changes the root blobstore prefix.

Arguments:
- `hostname`: Hostname.
- `root`: New blobstore key prefix. First component must match the ID of the project this hostname belongs to.

#### Get
Returns info about the hostname.

Arguments:
- `hostname`: Hostname.

Result `object`: An object containing info about the requested hostname.

