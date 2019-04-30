# System Configuration over Bluetooth GATT Service

## Overview

This library provides a way to examine and change configuration over Generic Attribute Service (GATT) Bluetooth Low-Energy (BLE) service.

The service is designed to be usable with any generic BLE mobile app that supports GATT, e.g. BLE Scanner ([Android](https://play.google.com/store/apps/details?id=com.macdom.ble.blescanner), [iOS](https://itunes.apple.com/us/app/ble-scanner-4-0/id1221763603)).

*Note*: Default BT configuration is permissive. See https://github.com/cesanta/mongoose-os/libs/bt-common#security for a better idea.

## Attribute description

The service UUID is `5f6d4f53-5f43-4647-5f53-56435f49445f`, which is a representation of a 16-byte string `_mOS_CFG_SVC_ID_`.

The service defines 3 characteristics (attributes):

 * `306d4f53-5f43-4647-5f6b-65795f5f5f30 (0mOS_CFG_key___0)` - a write-only attribute that selects the configuration key to be operated on. The key is a string of components separated by period, e.g. `wifi.sta.ssid` or `debug.level`.
 * `316d4f53-5f43-4647-5f76-616c75655f31 (1mOS_CFG_value_1)` - a read-write attribute that returns value of the selected key when read or accepts the value to be set. Note that value is not applied immediately after writing. Instead, consecutive writes to this key are appended to form the value to be set (this is due to small value of the default MTU and differences in how clients handle writes exceeding the MTU). For easy manual entry, attribute values are returned as strings and expected as strings as well. So, to enter value of 123, one should submit string `123`, not hex value `0x7b`. Boolean values should be entered as strings `true` or `false`.
 * `326d4f53-5f43-4647-5f73-6176655f5f32 (2mOS_CFG_save__2)` - a write-only attribute that applies value submitted to the value attribute to the key selected via the key attribute. The value written to this key can be one of:
   * `0` - just set the value, change will not be persisted
   * `1` - set and save the config
   * `2` - set, save and reboot

## Example - configuring WiFi

Here is an example of the configuration provisioning a device - configuring WiFi station settings (we'll use `key`, `value` and `save` to abbreviate long UUIDs):

  * Set the SSID, do not save the config yet
    * `wifi.sta.ssid` -> `key`
    * `MyNetwork` -> `value`
    * `0` -> `save`
  * Set the password, do not save the config yet
    * `wifi.sta.pass` -> `key`
    * `MyPassword` -> `value`
    * `0` -> `save`
  * Enable the station, save and reboot
    * `wifi.sta.enable` -> `key`
    * `true` -> `value`
    * `2` -> `save`

Once device is confirmed to have successfully connected to WiFi, BT config can be disabled:

  * `bt.config_enable` -> `key`  # Or `bt.enable` to disable Bluetooth entirely.
  * `false` -> `value`
  * `2` -> `save`  # Save and reboot with BT configuration disabled.

## See Also

See [rpc-gatts](https://github.com/cesanta/mongoose-os/libs/rpc-gatts) library,
which provides a GATT service that acts as an RPC channel. It accepts incoming
RPC frames and can send them as well - or rather,
makes them available for collection.
