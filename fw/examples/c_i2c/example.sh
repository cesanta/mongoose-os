#!/bin/sh
# sh example.sh "Hi there!"

MIOT_PORT=/dev/cu.SLAB_USBtoUART    # Module serial device
IP=192.168.1.22                     # Module IP address
#IP=$(miot call /v1/Config.GetNetworkStatus | perl -lne 'print $1 if /sta_ip": "(.+)"/')

MESSAGE=${1:-Hello World}
HEX_MESSAGE=$(echo -n $MESSAGE | xxd -g0 -p -c 9999)

# MCCXXX LCD setup
curl http://$IP/i2c?3e00391454726f0c01

# Show message
curl http://$IP/i2c?3e40$HEX_MESSAGE
