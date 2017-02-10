---
title: SSL/TLS setup
---

In order to setup SSL/TLS on the system HTTP server, create a certificate,
upload the certificate an the key file to the device, and change configuration.

Here is a procedure that creates a self-signed certificate:

```bash
openssl req  -nodes -new -x509  -keyout key.pem -out cert.pem
mos put cert.pem
mos put key.pem
mos config-set http.listen_addr=443 http.ssl_key=key.pem http.ssl_cert=cert.pem
mos config-set wifi.........   # Configure WiFi
curl -k https://IP_ADDRESS     # Test it !
```
