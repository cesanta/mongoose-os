---
title: Blink
---

This example shows how to get an LED to blink. It's a traditional "hello world"
application in the embedded world. It does not contain any logic to communicate
data to/from the outside world though. If you are looking for those, take a look at the
subsequent tutorials.

- Log in to [Mongoose Cloud](https://mongoose-iot.com).
- Create a new project, call it `blink`.
- Switch to the IDE tab.
- Copy/paste the following code into the `app.js`
- Adjust the PIN variable if necessary.

    ```javascript
// Pin selection suggestions.
//
// ESP8266:
//  - The blue LED on a ESP12(E) module is connected to GPIO2.
//  - A NodeMCU v2 board has an additional red LED connected to GPIO16.
//
// On a CC3200 LAUNCHXL board the red, amber and green LEDs are connected
// to pins 64, 1 and 2 respectively.

var PIN = 2;

function blink(pin, intervalMs) {
  var value = false;
  GPIO.setMode(pin, GPIO.OUT);
  setInterval(function() {
    value = !value;
    GPIO.write(pin, value);
  }, intervalMs);
}

console.log('Commence blinking pin ' + PIN + '...');
blink(PIN, 500);
    ```

- In the IDC (Interactive Device Console), choose your target device.
- Click the Flash button and wait until the hello message appears in the device log.
- LED will start to blink every 500ms.
