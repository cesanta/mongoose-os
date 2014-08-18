Embedded TCP/HTTP/Websocket server with Javascript API
======================================================

Smart.js is an embeddable C/C++ engine that allows to write plain TCP,
Web, or Websocket services entirely in Javascript. It is specifically suited
embedded environments with constrained resources, like smart devices,
telemetry probes, etс. Javascript interface makes development
extremely fast and effortless. Smart.js features include:

- Tiny footpint of
  [V7 Embedded Javascript Engine](http://github.com/cesanta/v7) and
  [Net Skeleton](http://github.com/cesanta/net_skeleton) makes Smart.js
  fit the most constrained environments
- Familiar Javascript interface makes development fast and available to a
  large group of engineers who already know Javascript,
  one of the [most popular](http://langpop.com) languages on the planet
- Simple and powerful
  [V7 C/C++ API](https://github.com/cesanta/v7/blob/master/v7.h)
  allows to interface with any other technology
  and delegate mission-critical tasks to the optimized compiled code
- Reference echo server applications let anybody with Javascript experience
  start developing in minutes
- Industry-standard security: native SSL/TLS support
- Smart.js is cross-platform and works on Windows, MacOS, iOS, UNIX/Linux,
  Android, QNX, eCos and many other platforms

## How to build and test Smart.js

On MacOS or UNIX/Linux, start terminal and execute the following commands:

    $ git clone https://github.com/cesanta/Smart.js.git
    $ cd Smart.js
    $ make CFLAGS_EXTRA="-DNS_ENABLE_SSL -lssl"
    $ ./engine

That starts the websocket echo server in your terminal.
To test it, open [echo.html](http://www.websocket.org/echo.html)
page in your browser, enter  `ws://localhost:8000` in the location field and
press "Connect".

## How to embed websocket.js into devices and applications

## Javascript interface documentation

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