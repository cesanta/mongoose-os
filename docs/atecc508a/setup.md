---
title: Setup procedure
---

Mongoose OS has native support for
[ATECC508A](http://www.atmel.com/devices/ATECC508A.aspx) crypto chip.
This section is a quick guide to get it up and running.
For a more detailed reference, especially of chip configuration, please
refer to Microchip documentation.

1. Generate a cert and key as normal. An example below shows a self-signed 
  certificate, but of course it doesn't have to be. The importnat thing is
  that it's a ECDSA certificate using P256 curve, since that is what the chip
  supports.

  ```
  $ openssl ecparam -out ecc.key.pem -name prime256v1 -genkey                                                                            
  $ openssl req -new -subj "/C=IE/L=Dublin/O=ACME Ltd/OU=Testing/CN=test.acme.com" -sha256 -key ecc.key.pem -text -out ecc.csr.tmpl
  $ openssl x509 -in ecc.csr.pem -text -out ecc.crt.pem -req -signkey ecc.key.pem -days 3650
  ```

2. Configure the chip. You can use our
  [sample configuration](https://raw.githubusercontent.com/cesanta/mongoose-os/master/fw/tools/atca-test-config.yaml).
  To set it, use extended `mos` commands:

  ```
  mos -X atca-set-config --port=/dev/ttyUSB0 atca-aws-test.yaml --dry-run=false
  mos -X atca-lock-zone --port=/dev/ttyUSB0 config --dry-run=false
  mos -X atca-lock-zone --port=/dev/ttyUSB0 data --dry-run=false
  ```

  Note: these changes are irreversible: once locked, zones cannot be
  unlocked anymore. Also, this sample config is very permissive and is only
  suitable for testing, NOT for production deployments. Please refer to 
  Microchip manual and other documentation to come up with more secure
  configuration (we may be able to assist with that too - ask a question
  on [our forum](http://forum.cesanta.com)).

3. Write the generated key into the device. Assuming you are using our
  sample configuration described in the previous section,
  this is a two-step process:

  3.1. Generate and set the key encryption key in slot 4

    ```
    $ openssl rand -hex 32 > slot4.key
    $ mos -X atca-set-key --port=/dev/ttyUSB0 4 slot4.key --dry-run=false
    AECC508A rev 0x5000 S/N 0x012352aad1bbf378ee, config is locked, data is locked
    Slot 4 is a non-ECC private key slot
    SetKey successful.
    ```

  3.2. Set the actual ECC key in slot 0

    ```
    $ mos -X atca-set-key --port=/dev/ttyUSB0 0 ecc.key.pem --write-key=slot4.key --dry-run=false
    AECC508A rev 0x5000 S/N 0x012352aad1bbf378ee, config is locked, data is locked

    Slot 0 is a ECC private key slot
    Parsed EC PRIVATE KEY
    Data zone is locked, will perform encrypted write using slot 4 using slot4.key
    SetKey successful.
    ```

4. Upload the certificate to the device

  ```
  $ mos put --port=/dev/ttyUSB0 ecc.crt.pem
  ```

5. Set HTTP server configuration to use the uploaded certificate and private
   key from device's slot 0:

  ```
  $ mos config-set --port=/dev/ttyUSB0 http.listen_addr=:443 http.ssl_cert=ecc.crt.pem http.ssl_key=ATCA:0
  Getting configuration...
  Setting new configuration...
  Saving and rebooting...
  ```

  At startup you should see in the device's log:

  ```
  mgos_sys_config_init_http HTTP server started on [443] (SSL)
  ```

  And when connecting with the browser:

  ```
  ATCA:2 ECDH get pubkey ok
  ATCA:0 ECDSA sign ok
  ATCA:2 ECDH ok
  ```
