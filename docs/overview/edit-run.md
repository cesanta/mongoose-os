---
title: Quick edit-flash-run cycle
---

[Mongoose Cloud](https://mongoose-iot.com) enables fast
edit-flash-run cycle. With a single button press, an IDE will:

- Build a firmware in seconds from sources.
- Copy built firmware to the device over-the-air, optimizing
  network transfer and flashing: copy/flash only parts that
  have changed.
- Run the new code, showing detailed logs in the console

No needs to fight hours and days with toolchain setup. Built
firmware can also be downloaded and flashed separately.

Our IDE allows to build a completely custom firmware with no
need to install any complicated toolchain on your machine.
You might want to use
[Mongoose Flashing Tool](https://github.com/cesanta/mft/releases)
once to flash the firmware for the first time on a stock device.

<img src="media/over_edit.png" width="100%">
