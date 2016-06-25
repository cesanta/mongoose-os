---
title: Blink
---

This example shows how to blink an LED.

- Login to the cloud IDE
- Copy/paste the following code into the `app.js`

    ```javascript
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

- Attach an LED to GPIO 2 and GND pins
- Build and Flash firmware to the device
- LED will start to blink each 500ms
