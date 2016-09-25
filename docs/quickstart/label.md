---
title: Label devices
---

Each registered device has a record on the cloud which stores device ID,
authentication information (secret key - password or certificate).

It is also possible to attach any number of `key=value` pairs to any device.
These labels can be used to store housekeeping information about a device.
One common use case is to select a list devices by certain label - and then
perform some action on them, for example update them remotely. That will
be shown in the next section.

Now, let's see how to label a device. First, let's get a list of
registered devices:

```sh
$ miot cloud dev list
device: d_JiFZ (created_at=1473921273, last_seen_at=1474790052)
```

What you can see inside the braces - are labels. The `created_at` label is
a UTC timestamp of the device registration time. That label is added to the
device by the registration process. Another label, `last_seen_at`, is
added by the cloud itself, when a device makes a connection to the cloud.
Therefore, devices with no `last_seen_at` label have never connected to
the cloud.

Let's add our own labels:

```sh
$ miot cloud label add d_JiFZewycny22znpexAygxA city=Dublin
$ miot cloud dev list
device: d_JiFZ (city=Dublin, created_at=1473921273, last_seen_at=1474790052)
```
