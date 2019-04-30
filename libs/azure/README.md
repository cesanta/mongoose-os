# Azure IoT Hub support library for Mongoose OS

This library provides [Azure IoT Hub](https://docs.microsoft.com/en-us/azure/iot-hub/) support for Mongoose OS.

Currently only plain MQTT is supported.

See Azure IoT + Mongoose OS tutorial at https://mongoose-os.com/docs/quickstart/cloud/azure.md

## Authentication

Authentication by both SAS token and X.509 certificate is supported. See the [Authentication section](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-security#authentication) of the documentation for explanation.


### `mos azure-iot-setup`

The easiest way to setup Azure cloud connection is by using `mos azure-iot-setup`. Makes sure you have the `az` CLI tool installed, create an IoT Hub, then run:
```
$ mos azure-iot-setup --azure-hub-name MY-HUB-NAME --azure-device-id NEW-DEVICE-ID
```

### SAS Token

To use symmetric key authentication, obtain the connection string from the web interface or by using the `az` CLI utility:
```
$ az iot hub device-identity show-connection-string --hub-name my-hub --device-id test1
{
  "cs": "HostName=my-hub.azure-devices.net;DeviceId=test1;SharedAccessKey=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA="
}
```

Enable the Azure client and set the `azure.cs` config setting:
```
$ mos config-set azure.enable=true "azure.cs=HostName=my-hub.azure-devices.net;DeviceId=test1;SharedAccessKey=AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA="
```

### X.509 Certificate

To use authentication by an X.509 certificate, upload the certificate and private key files in PEM format on the device and configure `azure.host_name`, `azure.device_id`, `azure.cert` and `azure.key`:

```
$ mos put test4.crt.pem
$ mos put test4.key.pem
$ mos config-set azure.enable=true azure.host_name=my-hub.azure-devices.net azure.device_id=test4 \
                 azure.cert=test4.crt.pem azure.key=test4.key.pem

```

_Note:_ It is possible to store private key in a cryptochip, such as [ATECC508A](http://www.microchip.com/wwwproducts/en/ATECC508A) (for example, as described [here](https://mongoose-os.com/blog/mongoose-os-google-iot-ecc508a/) for Google IoT Core). Just specify `azure.key=ATCA:0` to use private key in slot 0 of the chip. [mos azure-iot-setup] supports ATECC508 key storage - just add `--use-atca` to the setup command above.
