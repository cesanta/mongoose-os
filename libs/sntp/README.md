# SNTP library for Mongoose OS

When included to a Mongoose OS application, this library fetches the current
time from the SNTP server (by default, `time.google.com` is used) every time
the Internet connection is established, and adjusts the device time.

See `mos.yml` for the SNTP configuration available.
