---
title: Configuration
---

The system HTTP server has the following configuration options (we've added
comments to the `mos` tool output):

```bash
mos config-get http
{
  "enable": true,         # Set to false to disable default HTTP server
  "hidden_files": "",     # Glob pattern for files to hide from serving
  "listen_addr": "80",    # Port to listen on
  "ssl_ca_cert": "",      # CA certificate for mutual TLS authentication
  "ssl_cert": "",         # Certificate file
  "ssl_key": "",          # Private key file
  "upload_acl": "*"       # ACL for which files can be uploaded via /upload
}
```

For example, in order to change listening address to `8000`, do

```bash
mos config-set http.listen_addr=8000
```
