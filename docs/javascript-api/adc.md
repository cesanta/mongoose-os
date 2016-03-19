---
title: ADC
---

- `ADC.read(pin) -> number`: Return raw reading of the ADC device for a given
  analog input pin.

- `ADC.readVoltage(pin) -> number`: Read a voltage from the analog pin. Please
  read the platform specific manual for how to calibrate.

Example:

```javascript
function adcExample() {
  print("ADC:", ADC.read(0));
  setTimeout(adcExample, 1000);
}
```

