---
title: Device files
---

Mongoose Firmware provides a filesystem with POSIX interface. Due to the
implementation simplicity, a filesystem is flat - i.e. there are no directories.

`mos` tool makes it easy to view and upload files on a device. Use
`mos ls` to see a list of files on a device, `mos get FILE` to print
the contents of the FILE, and `mos put FILE` to copy FILE to the device:

```
$ mos ls
ca.pem
index.html
...
$ echo превед > a.txt
$ mos put a.txt
$ mos get a.txt
превед
```
