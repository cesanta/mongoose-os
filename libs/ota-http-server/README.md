# Implementation of Mongoose OS OTA HTTP server

## Overview

This library adds several HTTP endpoints that allow to `POST` new firmware
to the device over HTTP:

- `/update` - accept new firmware uploads via HTTP `POST`.
- `/update/revert` - roll back to the previous firmware.
- `/update/commit` - commit new firmware.

Example using `curl` tool (use Mac/Linux terminal or Windows command prompt).
Assume you have build a new firmware for your app. The zip file with a
built firmware is located at `build/fw.zip`. In order to update a live
device with IP address `IP_ADDRESS`, do:

```
$ curl -i -F filedata=@./build/fw.zip  http://IP_ADDRESS/update
HTTP/1.1 200 OK
Server: Mongoose/6.10
Content-Type: text/plain
Connection: close

Update applied, finalizing
```
