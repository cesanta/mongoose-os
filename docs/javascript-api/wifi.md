---
title: Wifi
---

By default, Wifi module enables access point mode, and acts as a DHCP server
for it.

- `Wifi.setup("yourssid", "youpassword") -> true or false`: Connect to the
  local Wifi network
- `Wifi.status() -> status_string`: Check current Wifi status
- `Wifi.ip() -> ip_address_string`: Get assigned IP address.  `Wifi.ip(1)`
  returns IP address of the access point interface.
- `Wifi.show()`: Returns the current SSID
- `Wifi.changed(cb)`: Invokes `cb` whenever the connection status changes:
  - `Wifi.CONNECTED`: connected
  - `Wifi.DISCONNECTED`: disconnected
  - `Wifi.GOTIP`: got ip
- `Wifi.scan(cb)`: Invoke `cb` with a list of discovered networks.
