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
  - 0: connected
  - 1: disconnected
  - 2: authmode changed
  - 3: got ip
  - 4: client connected to ap
  - 5: client disconnected from ap
- `Wifi.mode(mode) -> true or false`: Set Wifi mode. `mode` is a number, 1 is
  station, 2 is soft-AP, 3 is station + soft-AP
- `Wifi.scan(cb)`: Invoke `cb` with a list of discovered networks.

