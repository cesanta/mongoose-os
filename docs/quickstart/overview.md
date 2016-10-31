---
title: Overview
---

For the most impatient, this is a 20-second overview of the following
quick start guide:

```bash
# Initialise empty directory with firmware skeleton sources
miot init --arch ARCHITECTURE   # ARCHITECTURE can be cc3200 or esp8266

# Build the firmware on the Mongoose Cloud
miot build --user YOUR_USERNAME --pass YOUR_PASSWORD

# Flash built firmware onto the device
miot flash -port /dev/ttyUSB0 -fw build/fw.zip

# Configure WiFi on the device
miot config-set -port /dev/ttyUSB0 \
    wifi.ap.enable=false \
    wifi.sta.enable=true \
    wifi.sta.ssid=WIFI_NETWORK_NAME \
    wifi.sta.pass=WIFI_PASSWORD

# Register the device on the Mongoose Cloud
miot register -port /dev/ttyUSB0 \
    --user YOUR_USERNAME \
    --pass YOUR_PASSWORD \  
    --devide-id DEVICE_ID \
    --device-pass DEVICE_PASWORD
```

For those who would like to have more explanations, read along.
