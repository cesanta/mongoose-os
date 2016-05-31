---
title: Frame structure
---

Each frame is a JSON object with a predefined set of keys:

- `v`: frame format version, must be set to 2.
- `src`: source address.
- `dst`: destination address.
- `key`: optional pre-shared key (sometimes called API key) used to verify that
  the sender is really who he claims to be.
- `id`: ID of the request or response.

For requests:
- `method`: method name.
- `args`: method arguments, type and format are method-dependent.<br>
Timeout can be specified in two ways:
- `deadline`: as an absolute UNIX timestamp, or
- `timeout`: as a number of seconds (will be converted to `deadline` by the dispatcher)

For responses:
- `result`: result of the method invocation; type and format vary.
- `error`: presence of this key indicates an error. If present, must be an object containing two keys:
  - `code`: numeric error code, != 0
  - `message`: error message
