---
title: OTA RPC service
---

Mongoose OS provides OTA functions as an RPC service:

- `OTA.GetBootState` - returns current boot state. Example:

  ```
  $ mos --port ws://IP_ADDRESS/rpc call OTA.GetBootState
  {
    "active_slot": 0,
    "is_committed": true,
    "revert_slot": 1,
    "commit_timeout": 0
  }
  ```

- `OTA.SetBootState` - sets boot state. Example:
  ```
  $ mos --port ws://IP_ADDRESS/rpc call OTA.SetBootState \
    '{"is_committed": false, "revert_slot": 1, "commit_timeout": 300}'
  ```

- `OTA.Revert` - roll back to the previous firmware
- `OTA.Commit` - mark current firmware as OK
- `OTA.Update` - perform an OTA update

