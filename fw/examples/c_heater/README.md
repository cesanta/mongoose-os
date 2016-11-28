# Smart heater example

This is an example of the smart heater we use in our office. It provides
a number of HTTP endpoints to get heater status and perform a remote control:

- `/heater` : Shows a page with current status
- `/heater/on` : Turns a heater on
- `/heater/off` : Turns a heater off
- `/debug` : Shows current time and free RAM

Also, a heater is equipped with a
[MCP9808](http://www.microchip.com/wwwproducts/en/en556182) temperature sensor,
which is able to send temperature reading at regular intervals to
[Mongoose Cloud](http://mongoose.cloud). Cloud dashboard could be configured
to graph temperature data. You can see a graph of a temperature in our office
at this [dashboard](http://mongoose.cloud/login?user=test&pass=test).

A cloud endpoint and report interval can be specified via custom
configuration options, which is a nice example on how to use configuration
rather than hardcoding constants in the code.

See [quick start guide](https://mongoose-iot.com/docs/#/quickstart/)
on how to build and flash the firmware. Below is an example session that
builds and sets up a heater:

```bash
miot build        # Build the firmware
miot flash        # Flash the firmware
miot register     # Register device at Mongoose Cloud
miot config-set hsw.sensor_data_url=http://mongoose.cloud/api/DEVICE-USER/data/add
miot config-set hsw.auth="Bearer USER:PASSWORD"
miot config-set wifi.ap.enable=false wifi.sta.enable=true \
  wifi.sta.ssid=WIFI_NETWORK_NAME wifi.sta.pass=WIFI_PASSWORD
miot console      # Attach a serial console and see device logs
```
