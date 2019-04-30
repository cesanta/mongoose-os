# RPC Service - GPIO

This service provides an ability to manage GPIO on devices remotely.
It is possible to call this service programmatically via serial, HTTP/RESTful,
Websocket, MQTT or other transports
(see [RPC section](/docs/mongoose-os/userguide/rpc.md)) or use `mos` tool.

Below is a list of exported RPC methods and arguments:

## GPIO.Read
Set given pin in INPUT mode, read GPIO pin, return its value. Arguments:
```javascript
{
  "pin": 15     // Required. Pin number.
}
```

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call GPIO.Read '{"pin": 0}'
{
  "value": 1
}</code></pre>


## GPIO.Write
Set given pin in OUTPUT mode, set GPIO pin. Arguments:
```javascript
{
  "pin": 15,    // Required. Pin number.
  "value": 0    // Required. Voltage level. Either 0 (low) or 1 (high).
}
```

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call GPIO.Write '{"pin": 2, "value": 0}'</code></pre>


## GPIO.Toggle
Set given pin in OUTPUT mode, toggle voltage level and return that level. Arguments:
```javascript
{
  "pin: 15     // Required. Pin number.
}
```

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call GPIO.Toggle '{"pin": 2}'
{
  "value": 1
}</code></pre>


<!--

## GPIO.SetIntHandler
Set interrupt handler on a GPIO pin that calls a remote RPC service
on interrupt. Arguments:
```javascript
{
  "pin": 15,            // Required. Pin number.
  "edge": "any",        // Required. One of: "pos", "neg", "any".
  "pull": "up",         // Required. One of: "up", "down".
  "debounse_ms": 200,   // Required. Button debounce interval in milliseconds.
  "dst": "",            // Required. Remote RPC service destination address.
  "method": "Foo"       // Required. RPC method to call.
}
```

Example usage:
<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call GPIO.Toggle '{"pin": 2}'
{
  "value": 1
}</code></pre>
-->
