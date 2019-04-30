# Debug over Bluetooth GATT Service

## Overview

This library provides various debug functions over Generic Attribute Service (GATT) Bluetooth Low-Energy (BLE) service.

The service is designed to be usable with any generic BLE mobile app that supports GATT, e.g. BLE Scanner ([Android](https://play.google.com/store/apps/details?id=com.macdom.ble.blescanner), [iOS](https://itunes.apple.com/us/app/ble-scanner-4-0/id1221763603)).

*Note*: Default BT configuration is permissive. See https://github.com/cesanta/mongoose-os/libs/bt-common#security for a better idea.

## Attribute description

The service UUID is `5f6d4f53-5f44-4247-5f53-56435f49445f`, which is a representation of a 16-byte string `_mOS_DBG_SVC_ID_`.

At present, only one characteristic is defined:

* `306d4f53-5f44-4247-5f6c-6f675f5f5f30 (0mOS_DBG_log___0)` - a read/notify attribute that returns last debug log record when read. It also sends notifications with log messages as they are printed.
   * _Note_: Reading large messages is supported, but for notificatiosn to be useful you will most likely want to set higher MTU.
