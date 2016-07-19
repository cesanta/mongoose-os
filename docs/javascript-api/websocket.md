---
title: WebSocket
---

Mongoose IoT Platform provides WebSocket client functionality. The API follows the
standard, the same way it works in any browser.  Please refer to the [API
Reference](https://developer.mozilla.org/en-US/docs/Web/API/WebSocket) for the
detailed reference. Below is a usage example:

```javascript
var ws = new WebSocket('ws://echo.websocket.org');

ws.onopen = function(ev) {
  ws.send('hi');
};

ws.onmessage = function(ev) {
  print('Received WS data: ', ev.data);
  ws.close();
};

```
