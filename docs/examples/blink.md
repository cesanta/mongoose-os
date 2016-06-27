---
title: Blink
---

This example shows how to blink an LED. It's a traditional "hellow world"
application in embedded world. It does not contain any logic to communicate
data to/from the outside world though - for those, take a look at
subsequent tutorials.

- Login to [Mongoose Cloud](https://mongoose-iot.com)
- Create a new project, call it `blink`
- Swith to the IDE tab
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

- In the IDC (Interactive Device Console), choose your target device
- Attach an LED to GPIO 2 and GND pins
- Click Flash button, and wait until hello message appears in the device log
- LED will start to blink each 500ms
