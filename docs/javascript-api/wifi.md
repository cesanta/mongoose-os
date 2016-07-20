---
title: WiFi
---

By default, the WiFi module enables the access point mode and acts as a DHCP server
for it.

- `Wifi.setup("yourssid", "youpassword") -> true or false`: Connects to the
  local WiFi network
- `Wifi.status() -> status_string`: Checks current WiFi status
- `Wifi.ip() -> ip_address_string`: Gets the assigned IP address.  `Wifi.ip(1)`
  returns the IP address of the access point interface.
- `Wifi.show()`: Returns the current SSID
- `Wifi.changed(cb)`: Invokes `cb` whenever the connection status changes:
  - `Wifi.CONNECTED`: connected
  - `Wifi.DISCONNECTED`: disconnected
  - `Wifi.GOTIP`: got ip
- `Wifi.scan(cb)`: Invoke `cb` with a list of discovered networks.
