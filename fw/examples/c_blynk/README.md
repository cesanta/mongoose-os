# MongooseOS and Blynk example

This example shows how to use MongooseOS with Blynk framework.

See [quick start guide](https://mongoose-os.com/docs/#/quickstart/)
on how to build and flash the firmware. Below is an example session:

```bash
mos build --arch esp8266
mos flash
mos wifi SSID PASS
mos config-set blynk.auth=YOUR_TOKEN
mos
```

In the Blynk phone app, add a graph that reads a virtual pin 1,
and a toggle button that writes to a virtual pin 2. Toggling a button will
turn a built-in NodeMCU LED on/off, and a graph will show free RAM.
