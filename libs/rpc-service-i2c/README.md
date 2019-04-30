# RPC Service - I2C

This service provides an ability to manage I2C peripherals on devices remotely.
It is possible to call this service programmatically via serial, HTTP/RESTful,
Websocket, MQTT or other transports
(see [RPC section](/docs/mongoose-os/userguide/rpc.md)) or via the `mos` tool.

Below is a list of exported RPC methods and arguments:

## I2C.Scan
Scan the I2C bus, return a list of available device addresses. Arguments: none.

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call I2C.Scan
[
  31    # There is only 1 device with address 31 attached to the I2C bus
]</code></pre>


## I2C.Read
Read data from the I2C device. Arguments:
```javascript
{
  "addr: 31,    // Required. Device address.
  "len": 2      // Required. Number of bytes to read.
}
```

Reply:
```javascript
{
  // Hex-encoded data. Each byte is encoded as XX hex code, e.g. 0x00 0x1d:
  "data_hex": "001d"
}
```

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call I2C.Read '{"addr": 31, "len": 2}'
{
  "data_hex": "001d"
}</code></pre>


## I2C.Write
Write data to the I2C device. Arguments:
```javascript
{
  "addr: 31,              // Required. Device address.
  "data_hext": "1f3c6a"   // Required. Hex-encoded data to write
}
```

Example usage (showing failed write):

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call I2C.Write '{"addr": 31, "data_hex": "1f3c6a"}'
Error: remote error: I2C write failed</code></pre>


## I2C.ReadRegB
Read 1-byte register value. Arguments:
```javascript
{
  "addr: 31,  // Required. Device address.
  "reg": 0    // Required. Register number.
}
```

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call I2C.ReadRegB '{"addr": 31, "reg": 0}'
{
  "value": 0
}</code></pre>


## I2C.WriteRegB
Write 1-byte register value. Arguments:
```javascript
{
  "addr: 31,    // Required. Device address.
  "reg": 0,     // Required. Register number.
  "value": 0    // Required. 1-byte value to write.
}
```

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call I2C.WriteRegB '{"addr": 31, "reg": 0, "value": 0}'</code></pre>

## I2C.ReadRegW
Same as `I2C.ReadRegB`, but read 2-byte (word) register value.

## I2C.WriteRegW
Same as `I2C.WriteRegB`, but write 2-byte (word) register value.
