---
title: Device files
---

Mongoose Firmware provides a filesystem with POSIX interface. Due to the
implementation simplicity, a filesystem is flat - i.e. there are no directories.

`mgos` tool makes it easy to view and upload files on a device. Use
`mgos ls` to see a list of files on a device, `mgos get FILE` to print
the contents of the FILE, and `mgos put FILE` to copy FILE to the device:

```
$ mgos ls
ca.pem
index.html
...
$ echo превед > a.txt
$ mgos put a.txt
$ mgos get a.txt
превед
```
