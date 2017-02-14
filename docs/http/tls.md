---
title: SSL/TLS setup
---

In order to setup one-way SSL/TLS on the system HTTP server, create a certificate,
upload the certificate an the key file to the device, and change HTTP server configuration.

Here is a procedure that creates a self-signed certificate:

```bash
openssl req  -nodes -new -x509  -keyout key.pem -out cert.pem
mos put cert.pem
mos put key.pem
mos config-set http.listen_addr=443 http.ssl_key=key.pem http.ssl_cert=cert.pem
mos config-set wifi.........   # Configure WiFi on a device
curl -k https://IP_ADDRESS     # Test it !
```

If you want to use mutual (two-way) TLS with the device, follow this procedure to
use a self-signed certificate:

```bash
# Common parameters
SUBJ="/C=IE/ST=Dublin/L=Docks/O=MyCompany/CN=howdy"

# Generate CA
openssl genrsa -out ca.key 2048
openssl req -new -x509 -days 365 -subj $SUBJ -key ca.key -out ca.crt

# Generate client cert
openssl genrsa -out client.key 2048
openssl req -new -key client.key -out client.csr -subj $SUBJ
openssl x509 -req -days 365 -in client.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out client.crt

# Generate server cert
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr -subj $SUBJ
openssl x509 -req -days 365 -in server.csr -CA ca.crt -CAkey ca.key -set_serial 01 -out server.crt

# Upload server key, cert & ca cert to the device
mos put ca.crt
mos put server.key
mos put server.crt

# Update HTTP server settings to use mutual TLS
mos config-set http.ssl_ca_cert=ca.crt http.ssl_cert=server.crt http.ssl_key=server.key http.listen_addr=443
```

From that point on, the device should be accessible via secure Websocket:

```bash
mos config-set wifi.........   # Configure WiFi on a device
mos --cert-file client.crt --key-file client.key --port wss://IPADDR/rpc call RPC.List
```
