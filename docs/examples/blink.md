---
title: Blink example
---

This example shows how to blink an LED.

- Store this JavaScript snippet into a file `blink.js`

```javascript
function blink(pin) {
  pin = pin || 0;
  var level = GPIO.read(pin);
  GPIO.setmode(pin, 0, 0);
  GPIO.write(pin, !level);
  console.log("pin:", pin, ", level:", level);
  setTimeout(function() { blink(pin); }, 500);
}
```

- Upload file to the device: start FlashNChips, go to File/Upload, choose `blink.js`
- Attach an LED to GPIO 2 and GND pins
- In the console, write:

```javascript
File.eval("blink.js");
blink(2);
```

- LED will start to blink each 500ms.

