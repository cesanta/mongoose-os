Embedded JavaScript engine for C/C++
====================================

Smart.js is an embeddable JavaScript engine for C/C++. It is specifically suited
embedded environments with constrained resources, like smart devices,
telemetry probes, etс. JavaScript interface makes development
extremely fast and effortless. Smart.js features include:

- Network, File, Database, Crypto, OS API
- Simple and powerful
  [V7 C/C++ API](https://github.com/cesanta/v7/blob/master/v7.h)
  allows to easily export existing C/C++ functionality into JavaScript
- Industry-standard security: native SSL/TLS support
- Tiny footpint of
  [V7 Embedded JavaScript Engine](http://github.com/cesanta/v7) and
  [Net Skeleton](http://github.com/cesanta/net_skeleton) makes Smart.js
  fit the most constrained environments
- Smart.js is cross-platform and works on Windows, MacOS, iOS, UNIX/Linux,
  Android, QNX, eCos and many other platforms

## How to build and test Smart.js

On MacOS or UNIX/Linux, start terminal and execute following commands:

    $ git clone https://github.com/cesanta/Smart.js.git
    $ cd Smart.js
    $ make run


## Example: send periodic measurements to the cloud

    while (true) {
      var data = measure();  // Do some measurement
      var sock = Net.connect('ssl://mysite.com:443');
      Net.send('POST /api/data HTTP/1.0\n\ndata=' + data);
      Net.close(sock);
      Std.sleep(10);  // Sleep 10 seconds before the next cycle
    }

## Example: run HTTP/Websocket server

    var em = Net.EventManager()
    var web_server = Net.HttpServer({
      on_http_request: function(request, response) {
        response.send('URI was: ', request.uri);
      },
      on_websocket_message: function(message, response) {
        response.send(message);  // Echo websocket message back
      }
    });

    var sock = Net.bind('tcp://80')
    Net.ns_add(sock, web_server)

    while (true) {
      em.poll(1)  // Poll interval is 1 second
    }

## JavaScript API reference

<!-- ### Crypto API -->

<dl>
  <dt>Crypto.md5(str) -> (string) hash</dt>
  <dd>Calculates and returns 16-byte MD5 hash of a string parameter `str`</dd>

  <dt>Crypto.md5_hex(str) -> (string) hex_hash</dt>
  <dd>Calculates and returns 32-byte
    hex-stringified MD5 hash of a string parameter `str`</dd>

  <dt>Crypto.sha1(str) -> (string) hash</dt>
  <dd>Calculates and returns 20-byte SHA-1 hash of a string parameter `str`</dd>

  <dt>Crypto.sha1_hex(str) -> (string) hex_hash</dt>
  <dd>Calculates and returns 40-byte hex-stringified SHA-1 hash of
  a string parameter `str`</dd>

</dl>


<!-- ### Database API -->

<dl>
  <dt>Sqlite3.exec(sql, param1, ..., callback) -> (number) last_insert_id</dt>
  <dd>Executes SQL statement with given parameters,
    calling callback function for each row. Return: last insert ID</dd>

<!-- ### File API -->

<dl>
  <dt>File.fopen(file_name, mode) -> (number) file_stream</dt>
  <dd>
    Opens a file `file_name` with given `mode`. Valid values for `mode`
    are the same as for the
    [fopen](http://www.unix.com/man-page/posix/3/fopen/) POSIX call.
    Return value: numeric file stream, or negative OS error.

  <dt>File.fwrite(fp, data_str) -> (number) bytes_written</dt>
  <dd>Writes dat to a file stream.
    Return value: number of bytes written.</dd>

  <dt>File.fread(fp, num_bytes) -> (string) data</dt>
  <dd>Reads a data from file stream.
    Return: string with data. Upon EOF, empty string is returned.</dd>

  <dt>File.fclose(fp)</dt>
  <dd>Closes opened file stream. Return value: none.
    Note: failure to close will leak file descriptors.</dd>

  <dt>File.fseek(fp, offset, whence) -> (number) file_position</dt>
  <dd>Sets file position. Valid numeric values for `whence` are the same
  as for the `fseek` POSIX call.
  Return: result numeric position on success,
  or negative OS error code on failure. </dd>
</dl>


<!-- ### OS API -->
<dl>
  <dt>Std.exec(binary_path, arg1, arg2, ...) -> (number) exit_code</dt>
  <dd>Executes a given binary with arguments. Returns numeric exit code.</dd>

  <dt>Std.sleep(seconds)</dt>
  <dd>Sleeps for a given number of seconds. Seconds could be fractional,
  e.g. `0.1` for 1/10 of a second. Return value: none.</dd>

</dl>

<!-- ### Networking API -->

<dl>

  <dt>Net.bind(address) -> (number) listening_socket</dt>
  <dd>Creates and returns a listening socket bound to `address`. Format of
  `address` string is as follows: `[PROTO://][IP:]PORT[:SSL_CERT][CA_CERT]`.
  PROTO could be `tcp://`, `udp://` or `ssl://`. If omitted, then
  `tcp://` is assumed by default. IP could be an IP address or host name,
  in which case listening socket will be bound to a specific interface.
  If IP is omitted, then socket is bound to `INADDR_ANY`. PORT is a port
  number. For `ssl://` protocol, SSL_CERT specifies server certificate,
  which should be a file in PEM format with both SSL certificate and
  private key file concatenated. If CA_CERT is specified, then server will
  request client certificate (so called two-way SSL).</dd>

  <dt>Net.connect(address) -> (number) connected_socket</dt>
  <dd>Creates and returns a client socket to a specified address. The format
  of `address` is the same as for the `Net.bind()` function, where IP address
  is required. IP could be a host name. Optional SSL_CERT is a client
  certificate, in case if server configured for two-way SSL.
  If CA_CERT is specified, it is a CA certificate to verify
  server's identity.</dd>

  <dt>Net.send(sock, data) -> (number) num_bytes</dt>
  <dd>Writes data to a socket. Returns number of bytes written, or
  negative OS error in case of error.</dd>

  <dt>Net.recv(sock, num_bytes) -> (string) data</dt>
  <dd>Reads not more then `num_bytes` bytes of data data from a socket.
  Return: a string, an empty string on error.</dd>

  <dt>Net.close(sock)</dt>
  <dd>Closes opened socket.</dd>

  <dt>Net.EventManager() -> (object) event_manager</dt>
  <dd>Creates an asyncronous non-blocking event manager.</dd>



</dl>


## Licensing

This software is released under commercial and
[GNU GPL v.2](http://www.gnu.org/licenses/old-licenses/gpl-2.0.html) open
source licenses. The GPLv2 open source License does not generally permit
incorporating this software into non-open source programs.
For those customers who do not wish to comply with the GPLv2 open
source license requirements,
[Cesanta Software](http://cesanta.com) offers a full,
royalty-free commercial license and professional support
without any of the GPL restrictions.