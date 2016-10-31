---
title: Device files
---

Mongoose Firmware provides a filesystem with POSIX interface. Due to the
implementation simplicity, a filesystem is flat - i.e. there are no directories.

`miot` tool makes it easy to view and upload files on a device. Use
`miot ls` to see a list of files on a device, `miot get FILE` to print
the contents of the FILE, and `miot put FILE` to copy FILE to the device:

```
$ miot ls
ca.pem
index.html
...
$ echo превед > a.txt
$ miot put a.txt
$ miot get a.txt
превед
```
