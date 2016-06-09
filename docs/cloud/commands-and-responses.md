---
title: Commands and responses
---

Each command can have an integer ID (and it's automatically assigned if you don't
provide one), used to match response against it.

#### Command structure

- `cmd`: command name.
- `id`: optional numerical (positive integer, up to 2^53^-1) ID used to match
  the response.
- `args`: JSON object containing the command arguments.
- `deadline`: Unix timestamp of when the command result will be no longer
  relevant.

#### Response structure

- `id`: numerical ID of the command that generated this response.
- `status`: numerical status code, 0 indicates success, any non-zero value
  means error.
- `status_msg`: human-readable explanation of an error.
- `reponse`: the actual data returned by the command. Can be any JSON value
  except `null`.

