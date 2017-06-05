This example demonstrates how to use mJS Arduino Adafruit DHT library API
to get data from DHTxx temperature and humidity sensors.
Datasheet: https://cdn-shop.adafruit.com/datasheets/
Digital+humidity+and+temperature+sensor+AM2302.pdf

See [quick start guide](https://mongoose-os.com/docs/#/quickstart/)
on how to build and flash the firmware. Below is an example session:

```bash
mos build --arch ARCH        # Build the firmware. ARCH: esp8266, esp32, cc3200, stm32
mos flash                    # Flash the firmware
mos console                  # Attach a serial console and see device logs
```

See https://github.com/cesanta/mjs for the MJS engine information. 
