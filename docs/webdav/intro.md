---
title: Using WebDAV
---

Smart.js keeps files on flash using SPIFFS file system. In order to work with
files - edit, rename, save, make backups - Smart.js provides an easy way to do
so using WebDAV. Smart.js's networking core is
[Mongoose](https://github.com/cesanta/mongoose) which is able to do WebDAV and
much more.

First, boot the device - in AP or Station mode. Let's assume it gets and IP
address `192.168.0.16` in the local WiFi network - you can check that in the
Smart.js boot log, or for the AP mode, you can specify the address in the
configuration (`192.168.4.1` by default).

Then, use Windows Explorer or Mac Finder to mount the device. For Mac Finder,
start Finder, choose "Go -> Connect To Server" menu, and enter the IP address
of the device with `http://` prefix:

<img src="dav1.png" align="center"/>

When done, device's filesystem is accessible as an external disk:

<img src="dav2.png" align="center"/>

It is possible to add, remove, edit, rename files as usual, or work with them
with your favorite editor or IDE. The best experience we've seen is with
Sublime Text editor. Some editors like Vim create big swap files which can be
slow if the device is not fast enough.
