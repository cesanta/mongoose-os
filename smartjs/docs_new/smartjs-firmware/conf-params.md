---
title: Configuration parameters
---

- `wifi.sta.enable`: Enable or disable station. Enabling station disables AP
  unless `wifi.ap.keep_enabled` is set
- `wifi.sta.ssid`: SSID of WiFi network to connect
- `wifi.sta.pass`: Password of WiFi network to connect

- `wifi.ap.enable`: Enable or disable AP
- `wifi.ap.keep_enabled`: Keep AP enabled when station is on (if supported by
  the platform, e.g. ESP8266). Normally AP is turned off once station is
  configured
- `wifi.ap.trigger_on_gpio`: GPIO number, which should be grounded to force AP
  mode
- `wifi.ap.ssid`: SSID of created network
- `wifi.ap.pass`: Password for created network
- `wifi.ap.hidden`: Do not broadcast SSID
- `wifi.ap.hidden`: Network channel
- `wifi.ap.ip`: IP of the device in created network
- `wifi.ap.netmask`: Netmask of created network
- `wifi.ap.gw`: IP of gateway to use
- `wifi.ap.dhcp_start`: Clients IP range start
- `wifi.ap.dhcp_end`: Clients IP range end

- `http.enable`: Enable or disable configuration web-server
- `http.listen_addr`: Listen address for configuration web-server
- `http.enable_webdav`: Enable access to the device via WebDAV

- `update.server_timeout`: Timeout to use in network operations during update
  operation (seconds). Might be increased for slow networks.

- `clubby.connect_on_boot`: Enable or disable default clubby connection on boot
- `clubby.server_address`: Default clubby server address (including port)
- `clubby.device_id`: The device ID used to connect clubby server
- `clubby.device_psk`: The device PSK used to connect clubby server
- `clubby.device_auto_registration`: Allow automatical obtaining of ID and PSK
  from Cesanta server
- `clubby.device_registration_url`: Server to obtain ID and PSK
- `clubby.reconnect_timeout_min`, `clubby.reconnect_timeout_max`: Timeouts to
  use in clubby network operations (seconds). Timeout grows with every
  unsuccessful connection attempt, starting from `reconnect_timeout_min`,
  maximum value is `clubby.reconnect_timeout_max`
- `clubby.cmd_timeout`: Default life time for commands sent to server (seconds)
- `clubby.memory_limit`: Clubby can enqueue commands if connection is broken
  and send them once connection is restored. This feature will be disabled if
  free memory amount is less than `memory_limit` value (bytes)

- `debug.level`: Level of logs detail.
  - `0`: logs are disabled,
  - `1`: errors only,
  - `2`: errors and warnings
  - `3`: debug mode,
  - `4`: enhanced debug mode.
- `debug.stdout_uart`: Where to send normal (stdout) output.
  - `0`: UART0 (default),
  - `1`: UART`,
  - `-1`: `/dev/null`.
- `debug.stderr_uart`: Where to send diagnostic (stderr) output.
  - `0`: UART0,
  - `1`: UART` (default),
  - `-1`: `/dev/null`.
- `debug.enable_prompt`: Whether to enable interactive JavaScript prompt.
  Default: `true`.
- `debug.factory_reset_gpio`: GPIO number that will trigger a factory reset if
  held low during boot. Default: `-1`.
