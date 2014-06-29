Embedded HTML5 Websocket server with Javascript interface
=========================================================

To build and test it, start terminal and execute following commands:

    $ git clone https://github.com/cesanta/websocket.js.git
    $ cd websocket.js
    $ make CFLAGS_EXTRA="-DNS_ENABLE_SSL -lssl"
    $ ./engine

That builds and starts the websocket echo server.

To test it, open http://www.websocket.org/echo.html in your browser, and
specify `ws://localhost:8000` in the location field.

## How it works

## Embed websocket.js into devices and applications

## Licensing