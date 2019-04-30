# RPC Service - Config

This service provides an ability to manage device configuration remotely.
It is required by the `mos config-get` and `mos config-set` commands.
If this library is not included in the app, those commands won't work.
It is possible to call this service programmatically via serial, HTTP/RESTful,
Websocket, MQTT or other transports
(see [RPC section](/docs/mongoose-os/userguide/rpc.md)) or use `mos` tool.

<iframe src="https://www.youtube.com/embed/GEJngJxtTWw"
  width="560" height="315"  frameborder="0" allowfullscreen></iframe>

Below is a list of exported RPC methods and arguments:

## Config.Get
Get device configuration subtree. Arguments:

```javascript
{
  // Optional. Path to a config object, e.g. `wifi.sta.ssid`.
  // If not specified, a full configuration tree is returned.
  "key": "..."
}
```

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-6,8"><code>mos call Config.Get
{
  "http": {
    "enable": true,
    "listen_addr": "80",
    ...
mos call Config.Get '{"key": "wifi.sta.enable"}'
true</code></pre>

This RPC command has a shortcut: `mos config-get`:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-6,8"><code>mos config-get
{
  "http": {
    "enable": true,
    "listen_addr": "80",
    ...
mos config-get wifi.sta.enable
true</code></pre>

## Config.Set
Set device configuration parameters. Arguments:

```javascript
{
  // Required. Contains a sparse object with configuration parameters.
  // These parameters are applied on top of the existing device configuration.
  "config": { ... }
}
```

Example usage - set `debug.level` to `3`:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-6,8"><code>mos call Config.Set '{"config": {"debug": {"level": 3}}}'</code></pre>

This RPC command has a shortcut: `mos config-set` which sets the config
option, saves it, and reboots the device (since some config options take
effect only after reboot):

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-6,8"><code>mos config-set debug.level=3
Getting configuration...
Setting new configuration...
Saving and rebooting...</code></pre>

## Config.Save
Writes an existing device confuguration on flash, as a sequence of
`confX.json` files
(see [description](/docs/mongoose-os/userguide/configuration.md)). This makes
configuration permament, preserved after device reboot. Arguments:

```javascript
{
  "reboot": false  // Optional. Whether to reboot the device after the call
}
```

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-6,8"><code>mos call Config.Save '{"reboot": true}'</code></pre>
