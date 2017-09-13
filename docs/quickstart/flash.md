---
title: Install Mongoose OS
---

Now it's time to install Mongoose OS. There is a pre-built, default firmware
for all supported architectures, located on https://mongoose-os.com web site.
It contains a JavaScript engine, and runs a blink example. Start with
Mongoose OS by connecting a hardware module to your computer and
flashing that pre-built firmware:

```bash
mos flash esp8266  # Download a default firmware for ESP8266 and flash it
```

Alternatively, you can install and perform an initial configuration
using a Web UI:

<iframe width="560" height="315" align="center"
	src="https://www.youtube.com/embed/bDsqR6HBseY"
	frameborder="0" allowfullscreen></iframe>

Once the flashing is complete, you can either use command line tools to
edit files and upload them to the device, or use the Web UI in the
prototyping mode:

![](media/mos2.png)

You can customise the firmware using JavaScript, or build your completely
custom firmware in C, which is covered in the next section.