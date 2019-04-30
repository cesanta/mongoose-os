// Wifi global object is created during C initialization.

// ## **`Wifi.scan(cb)`**
// Scan WiFi networks, call `cb` when done.
// `cb` accepts a single argument `results`, which is
// either `undefined` in case of error, or an array of object containing:
// ```javascript
// {
//   "ssid": "NetworkName",
//   "bssid": "12:34:56:78:90:ab",
//   "authMode": Wifi.AUTH_MODE_WPA_PSK, // Auth mode, one of AUTH constants.
//   "channel": 11,
//   "rssi": -70
// }
// ```
// Example:
// ```javascript
// Wifi.scan(function(results) {
//   print(JSON.stringify(results));
// });
// ```

// Must be kept in sync with enum mgos_wifi_auth_mode
// ## **Auth modes**
// - `Wifi.AUTH_MODE_OPEN`
// - `Wifi.AUTH_MODE_WEP`
// - `Wifi.AUTH_MODE_WPA_PSK`
// - `Wifi.AUTH_MODE_WPA2_PSK`
// - `Wifi.AUTH_MODE_WPA_WPA2_PSK`
// - `Wifi.AUTH_MODE_WPA2_ENTERPRISE`
Wifi.AUTH_MODE_OPEN = 0;
Wifi.AUTH_MODE_WEP = 1;
Wifi.AUTH_MODE_WPA_PSK = 2;
Wifi.AUTH_MODE_WPA2_PSK = 3;
Wifi.AUTH_MODE_WPA_WPA2_PSK = 4;
Wifi.AUTH_MODE_WPA2_ENTERPRISE = 5;
