# I2C over HTTP example

The purpose of this example is to demonstrate how it is possible to
control an LCD screen with I2C interface remotely. The way it is done
is as follows:

1. Mongoose Firmware registers an HTTP handler that expects an ASCII sequence
   which is an encoded byte stream:

   ```Bash
    curl http://$MODULE_IP_ADDRESS/i2c?3e00391454726f0c01
   ```
2. Mongoose Firmware sends those bytes to the I2C bus. First byte is always
   a device address.

3. The reply is `{"status": 0}`, where 0 means all bytes were sent to the
   target device successfully. Any other error status means error.

The `example.sh` script assumes that the module has an MCCxxx LCD screen
attached to it (see datasheet at http://www.farnell.com/datasheets/1821133.pdf).
`example.sh` could be modified to send a different I2C sequence, thus other
LCD screens could be easily adopted by that generic I2C write handler.

## How to try it

1. Build and flash

```
miot build --arch esp8266
miot flash
miot config-set wifi.sta.enable=true wifi.ap.enable=false wifi.sta.ssid=XX wifi.sta.pass=YY
miot console
```

2. In another terminal,

```
# Change IP address in the example.sh to be your module's IP address
# which you can see in the logs
sh example.sh "hi there"
```
