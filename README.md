# Smart.JS platform

Smart.JS is a generic, hardware independent, full-stack
Internet of Things software platform.
Smart.JS solves problems of reliability, scalability, security
and remote management which are common to all verticals, being it industrial
automation, healthcare, automotive, home automation, or other.

# Overview

Technically, Smart.JS has a device part and a cloud part.

![](http://cesanta.com/images/smartjs_diagram.png)

Smart.JS firmware on a device side:

- allows scripting for fast and safe development & firmware update.
  We do that by developing world's smallest JavaScript scripting engine.
- provides hardware and networking API that guarantees reliability,
  scalability and security out-of-the-box.
- Devices with our software can be managed remotely and update software
  remotely, in a fully automatic or semi-automatic way.

Smart.JS software on a cloud side has three main components:

- device management database that keeps information about all devices,
  e.g. unique device ID, device address, software version, et cetera.
- telemetry database with analytics. It can, for example, store information
  from remote sensors, like electricity and water meters, and able to answer
  questions like "show me a cumulative power consumption profile for plants
  A and B from 3 AM to 5 AM last night."
- remote software update manager. Schedules and drives software updates
  in a reliable way. Understands policies like "start remote update with
  device ID 1234 always. Check success in 5 minutes. If failed, roll back
  to previous version and alert. If successful, proceed with 5 more random
  devices of the same class. If successful, proceed with the rest of devices.
  Never keep more than 5% of devices in flight. If more then 0.1% updates
  fail, stop the update globally, do not roll back, and alert."


# Supported device architectures

- Texas Instruments CC3200
- Espressif ESP8266

For burning Smart.JS firmware to devices, we provide a `stool` utility.
Click on [releases](releases) link to download it.

# Contributions

People who have agreed to the
[Cesanta CLA](http://cesanta.com/contributors_la.html)
can make contributions. Note that the CLA isn't a copyright
_assigment_ but rather a copyright _license_.
You retain the copyright on your contributions.

# Licensing

Mongoose is released under commercial and
[GNU GPL v.2](http://www.gnu.org/licenses/old-licenses/gpl-2.0.html) open
source licenses. The GPLv2 open source License does not generally permit
incorporating this software into non-open source programs.
For those customers who do not wish to comply with the GPLv2 open
source license requirements,
[Cesanta](http://cesanta.com) offers a full,
royalty-free commercial license and professional support
without any of the GPL restrictions.
