---
title: Field access control
---

Some settings in the configuration may be sensitive and the vendor may,
while providing a way for user to change settings, restrict certain fields
or (better) specify which fields can be changed by the user.

To facilitate that, the configuration system contains field access control,
configured by the `field access control list` (ACL).

- ACL is a comma-delimited list of entries which are applied to full field
  names when loading config files at boot time.
- ACL entries are matched in order and, search terminates when a match is found.
- ACL entry is a pattern, where `*` serves as a wildcard.
- ACL entry can start with `+` or `-`, specifying whether to allow or
  deny change to the field if the entry matches. `+` is implied but can
  be used for clarity.
- The default value of the ACL is `*`, meaning changing any field is allowed.

ACL is contained in the configuration itself - it's the top-level `conf_acl`
field. The slight twist is that during loading, the setting of the
_previous_ layer is in effect: when loading user settings,
`conf_acl` from vendor settings is consulted,
and for vendor settings the `conf_acl` value from the defaults is used.

For example, to restrict users to only being able change WiFi and debug level
settings, `"conf_acl": "wifi.*,debug.level"` should be set in `conf_vendor.json`.

Negative entries allow for default-allow behaviour:
`"conf_acl": "-debug.*,*"` allows changing all fields except anything under `debug`.
