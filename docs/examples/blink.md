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

    ```javascript
    console.log('Hello from blink example');
    function blink(pin) {
      pin = pin || 0;
      var level = GPIO.read(pin);
      GPIO.setMode(pin, 0, 0);
      GPIO.write(pin, !level);
      console.log("pin:", pin, ", level:", level);
      setTimeout(function() { blink(pin); }, 500);
    }
    blink(2);
    ```

- In the IDC (Interactive Device Console), choose your target device.
- Attach an LED to GPIO 2 and GND pins.
- Click the Flash button and wait until the hello message appears in the device log.
- LED will start to blink every 500ms.
