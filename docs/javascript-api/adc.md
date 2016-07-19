---
title: ADC
---

- `ADC.read(pin) -> number`: Returns raw reading of the ADC device for a given
  analog input pin.

- `ADC.readVoltage(pin) -> number`: Reads voltage from the analog pin. Please
  read the platform specific manual for how to calibrate it.

Example:

```javascript
function adcExample() {
  print("ADC:", ADC.read(0));
  setTimeout(adcExample, 1000);
}
```

*Note: On ESP8266 only TOUT pin (GPIO 6) can be used as analog input. `Pin` parameter is ignored on this platform.*
