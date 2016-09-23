---
title: Quick edit-flash-run cycle
---

[Mongoose Cloud](https://mongoose-iot.com) enables a fast
edit-flash-run cycle. With the push of a a single button, the IDE will:

- Build the firmware in seconds from sources.
- Copy the built firmware to the device over-the-air, optimising the
  network transfer and flashing: copy/flash are the only parts that
  have changed.
- Run the new code. It will show detailed logs in the console.

No need to fight for hours with toolchain setup. The built
firmware can also be downloaded and flashed separately.

Our IDE allows you to build completely custom firmware with no
need to install any complicated toolchain on your machine.
We recommend using [Mongoose Flashing Tool](https://github.com/cesanta/mft/releases)
once to flash the firmware for the first time on a stock device.

<img src="media/over_edit.png" width="100%">
