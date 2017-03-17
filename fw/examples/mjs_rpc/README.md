See [quick start guide](https://mongoose-os.com/docs/#/quickstart/)
on how to build and flash the firmware. Below is an example session:

```bash
export MOS_PORT=YOUR_DEVICE_SERIAL_PORT
mos build --arch ARCH        # Build the firmware. ARCH: esp8266, esp32, cc3200
mos flash                    # Flash the firmware
mos console                  # Attach a serial console and see device logs
```

See https://github.com/cesanta/mjs for the MJS engine information. 
