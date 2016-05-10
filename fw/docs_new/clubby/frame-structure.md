---
title: Frame structure
---

Each frame is a JSON object with a predefined set of keys:

- `v`: frame format version, must be set to 1.
- `src`: source address.
- `dst`: destination address.
- `key`: optional pre-shared key (sometimes called API key) used to verify that
  the sender is really who he claims to be.
- `cmds`: array of JSON objects, each carrying a name of the command to invoke
  and its arguments.
- `resp`: array of JSON objects containing responses to the commands.

