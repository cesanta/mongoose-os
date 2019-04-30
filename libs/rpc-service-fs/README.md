# RPC Service - Filesystem

This service provides an ability to manage device files remotely.
It is required by the `mos ls`, `mos get`, `mos put`, `mos rm` commands.
If this library is not included in the app, those commands won't work.
It is possible to call this service programmatically via serial, HTTP/RESTful,
Websocket, MQTT or other transports
(see [RPC section](/docs/mongoose-os/userguide/rpc.md)) or use `mos` tool.

<iframe src="https://www.youtube.com/embed/z5JltFNF_RE"
  width="560" height="315" frameborder="0" allowfullscreen></iframe>

Below is a list of exported RPC methods and arguments:

## FS.List
Get device file list. Arguments: none.

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call FS.List
[
  "mgos_ro_vars_schema.json",
  "conf0.json",
  ...
]</code></pre>

This RPC command has a shortcut: `mos ls`:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos ls
"mgos_ro_vars_schema.json",
"conf0.json",
...</code></pre>


## FS.ListExt
Same as `FS.List` but also returns extra file info like file sizes.
Arguments: none.

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call FS.ListExt
[
  {
    "name": "mgos_ro_vars_schema.json",
    "size": 332
  },
  ...
]</code></pre>

This RPC command has a shortcut: `mos ls -l`:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos ls -l
api_adc.js 259
api_arch_uart.js 651
...</code></pre>


## FS.Get
Returns file content. Arguments:
```javascript
{
  "filename": "foo.txt",    // Required. Name of the file to fetch.
  "offset": 0,              // Optional. Offset to begin with.
  "len": 100                // Optional. Number of bytes to return.
                            // If this is too large, the call may fail with OOM.
}
```

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call FS.Get '{"filename": "init.js"}'
{
  "data": "bG9hZCgnYXBpX2NvbmZpZ...",   # Base64 encoded data
  "left": 0                             # How many bytes left in a file
}</code></pre>

This RPC command has a shortcut: `mos get`:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos get init.js
load('api_gpio.js');
...</code></pre>


## FS.Put
Write data into file. Write is done either by overwriting an existing
content, or by appending to the existing content. Arguments:
```javascript
{
  "filename": "foo.txt",    // Required. Name of the file to write to.
  "append": false,          // Optional. Overwrite or append.
  "data": "base64 text"     // Required. Data to write.
}
```

This RPC command has a shortcut: `mos put`. It splits large files into
small chunks, and calls `FS.Put` sequentially, appending content.

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos put /etc/passwd foo.txt</code></pre>

## FS.Remove
Delete file. Arguments:
```javascript
{
  "filename": "foo.txt"    // Required. Name of the file to delete
}
```

This RPC command has a shortcut: `mos rm`:
<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos rm foo.txt</code></pre>


## FS.Mkfs
Create filesystem. Arguments:
```javascript
{
  "dev_type": "spi_flash",          // Required. Filesystem driver name.
  "dev_opts": "...",                // Required. Device-specific options.
  "fs_type": "SPIFFS",              // Required. Filesystem type.
  "fs_opts": "{\"site\": 131072}"   // Required. Filesystem-specific options.
}
```

Example usage:
<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call FS.Mkfs '{"dev_type": "spi_flash", "dev_opts": "{\"freq\": 20000000, \"cs\": 0}", "fs_type": "SPIFFS", "fs_opts": "{\"size\": 131072}'</code></pre>

## FS.Mount
Mount filesystem. Arguments:
```javascript
{
  "path": "/foo",           // Required. Directory name to attach to.
  "dev_type": "...",        // See FS.Mkfs above
  "dev_opts": "...",
  "fs_type": "SPIFFS",
  "fs_opts": "..."
}
```

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call FS.Mount '{"path": "/mnt", "dev_type": "spi_flash", "dev_opts": "{\"freq\": 20000000, \"cs\": 0}", "fs_type": "SPIFFS", "fs_opts": "{\"size\": 131072}"}'</code></pre>


## FS.Umount
Unmout filesystem. Arguments:
```javascript
{
  "path": "/foo"           // Required. Mount point to detach.
}
```

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call FS.Mount '{"path": "/mnt"}'</code></pre>
