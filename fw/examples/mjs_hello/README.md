See [quick start guide](https://mongoose-iot.com/docs/#/quickstart/)
on how to build and flash the firmware. Below is an example session:

```bash
export MGOS_PORT=YOUR_DEVICE_SERIAL_PORT
mgos build --arch ARCH        # Build the firmware. ARCH: esp8266, esp32, cc3200
mgos flash                    # Flash the firmware
mgos console                  # Attach a serial console and see device logs
```

See https://github.com/cesanta/mjs for the MJS engine information. 
