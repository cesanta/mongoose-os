# GATT Client RPC service

Provides [RPC service](https://mongoose-os.com/docs/mongoose-os/userguide/rpc.md) for
[GATT](https://learn.adafruit.com/introduction-to-bluetooth-low-energy/gatt)
client.

## Methods

 * `GATTC.Scan` - performs BLE scan of the area. Results are returned as an array of objects containing address, name (if present) and RSSI.
```
$ mos call GATTC.Scan
{
  "results": [
    {
      "addr": "eb:12:dd:51:19:3d",
      "rssi": -48
    },
    {
      "addr": "24:0a:c4:00:31:be",
      "name": "esp32_0031BC",
      "rssi": -36
    }
  ]
}
```

 * `GATTC.Open` - open connection to a device. Device can be specified by either `addr` or `name`. Optional `mtu` parameter specifies link MTU to be used, the default is 23 bytes (as per standard). Returned value is `conn_id` which is the connection identifier to be used for later operations.
```
$ mos call GATTC.Open '{"addr": "eb:12:dd:51:19:3d"}'
{
  "conn_id": 0
}
$ mos call GATTC.Open '{"name": "esp32_0031BC", "mtu": 200}'
{
  "conn_id": 1
}
```

 * `GATTS.ListServices` - list services provided by device. For each discovered service, its UUID, instance number and primary flag are returned.
```
$ mos call GATTC.ListServices '{"conn_id": 1}'
{
  "results": [
    {
      "uuid": "1801",
      "instance": 0,
      "primary": true
    },
    {
      "uuid": "1800",
      "instance": 0,
      "primary": true
    },
    {
      "uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f",
      "instance": 0,
      "primary": true
    },
    {
      "uuid": "5f6d4f53-5f52-5043-5f53-56435f49445f",
      "instance": 0,
      "primary": true
    }
  ]
}
```
  In this example, we see two standard services - [Generic Access](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.generic_access.xml) (1800) and [Generic Attribute](https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.service.generic_attribute.xml), followed by custom mOS [configuration](https://github.com/cesanta/mongoose-os/libs/bt-service-config) and [RPC over GATT](https://github.com/cesanta/mongoose-os/libs/rpc-gatts) services.

 * `GATTS.ListCharacteristics` - list characteristics of a service. For each charateristic, its UUID and properties are returned.
  Properties are returned a string of up to 8 characters:
    * `R` - read
    * `W` - write
    * `w` - write with no response required
    * `N` - notify
    * `I` - indicate
    * `B` - broadcast
    * `E` - extended
    * `A` - auth

```
$ mos call GATTC.ListCharacteristics '{"conn_id": 1, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f"}'
{
  "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f",
  "results": [
    {
      "uuid": "306d4f53-5f43-4647-5f6b-65795f5f5f30",
      "props": "W"
    },
    {
      "uuid": "316d4f53-5f43-4647-5f76-616c75655f31",
      "props": "WR"
    },
    {
      "uuid": "326d4f53-5f43-4647-5f73-6176655f5f32",
      "props": "W"
    }
  ]
}
```

 * `GATTC.Write` - write a value to the specified characteristic. `conn_id`, `svc_uuid` and `char_uuid` specify the characteristic, value can be provided either as plain text `value` or hex-encoded `value_hex` keys. The following two calls are equivalent
```
$ mos call GATTC.Write '{"conn_id": 1, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f", "char_uuid": "306d4f53-5f43-4647-5f6b-65795f5f5f30", "value": "wifi.ap.enable"}'
null
$ mos call GATTC.Write '{"conn_id": 1, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f", "char_uuid": "306d4f53-5f43-4647-5f6b-65795f5f5f30", "value_hex": "776966692e61702e656e61626c65"}'
null
```

 * `GATTC.Read` - read avlue of a characteristic `conn_id`, `svc_uuid` and `char_uuid` specify the characteristic, value will be returned as either `value` or `value_hex`: if value is a string consisting of printable charaters that can be represented as a valid JSON string, result will be in the `value` key. Otherwise it will be hexified and sent in `value_hex`.
```
$ mos call GATTC.Read '{"conn_id": 1, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f", "char_uuid": "316d4f53-5f43-4647-5f76-616c75655f31"}'
{
  "value": "true"
}
```

 * `GATTC.Close` - close the specified connection.

```
$ mos call GATTC.Close '{"conn_id": 1}'
null
```

## Provisioning one device using another
  As a complete example, here is full Bluetooth provisioning process described in [this blog post](https://mongoose-os.com/blog/bluetooth-support-for-esp32/):

```
$ mos call GATTC.Open '{"name": "esp32_0031BC", "mtu": 200}'
{
  "conn_id": 0
}
$ mos call GATTC.Write '{"conn_id": 0, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f", "char_uuid": "306d4f53-5f43-4647-5f6b-65795f5f5f30", "value": "wifi.sta.ssid"}'
null
$ mos call GATTC.Write '{"conn_id": 0, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f", "char_uuid": "316d4f53-5f43-4647-5f76-616c75655f31", "value": "Cesanta"}'
null
$ mos call GATTC.Write '{"conn_id": 0, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f", "char_uuid": "326d4f53-5f43-4647-5f73-6176655f5f32", "value": "0"}'
null
$ mos call GATTC.Write '{"conn_id": 0, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f", "char_uuid": "306d4f53-5f43-4647-5f6b-65795f5f5f30", "value": "wifi.sta.pass"}'
null
$ mos call GATTC.Write '{"conn_id": 0, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f", "char_uuid": "316d4f53-5f43-4647-5f76-616c75655f31", "value": "SECRET"}'
null
$ mos call GATTC.Write '{"conn_id": 0, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f", "char_uuid": "326d4f53-5f43-4647-5f73-6176655f5f32", "value": "0"}'
null
$ mos call GATTC.Write '{"conn_id": 0, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f", "char_uuid": "306d4f53-5f43-4647-5f6b-65795f5f5f30", "value": "wifi.sta.enable"}'
null
$ mos call GATTC.Write '{"conn_id": 0, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f", "char_uuid": "316d4f53-5f43-4647-5f76-616c75655f31", "value": "true"}'
null
$ mos call GATTC.Write '{"conn_id": 0, "svc_uuid": "5f6d4f53-5f43-4647-5f53-56435f49445f", "char_uuid": "326d4f53-5f43-4647-5f73-6176655f5f32", "value": "2"}'
null
```
