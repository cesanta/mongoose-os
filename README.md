[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)  [![Gitter](https://badges.gitter.im/cesanta/mongoose-os.svg)](https://gitter.im/cesanta/mongoose-os?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

# Mongoose OS - an IoT Firmware Development Framework

- Over-The-Air firmware updates and remote management - reliable updates with rollback on failures, remote device access infrastructure
- Security - 	built in flash encryption, crypto chip support, ARM mbedTLS optimized for small memory footprint
- [Device management dashboard service](https://mdash.net)
- Supported microcontrollers: CC3220, CC3200, ESP32, ESP8266, STM32F4, STM32L4, STM32F7
- Recommended dev kits: [ESP32-DevKitC for AWS IoT](https://mongoose-os.com/aws-iot-starter-kit/), [ESP32 Kit for Google IoT Core](https://mongoose-os.com/gcp/)
- Built-in integration for AWS IoT, Google IoT Core, Microsoft Azure, Adafruit IO, generic MQTT servers
- Code in C or JavaScript
- Ready to go Apps and Libraries
- [Embedded JavaScript engine - mJS](https://github.com/cesanta/mjs)


Trusted and Recommended By:
- Amazon AWS - [Amazon AWS Technology Partner](https://aws.amazon.com/blogs/apn/partner-sa-roundup-may-2017/)
- Google IoT Core - [Mongoose OS is a Google Cloud IoT Core Partner](https://cloud.google.com/iot/partners/)
- IBM Watson IoT - [Mongoose OS is a Ready for IBM Watson IoT validated solution](https://www-356.ibm.com/partnerworld/gsd/solutiondetails.do?solution=55930&lc=en&stateCd=P&tab=1)
- Microsoft Azure IoT - [Mongoose OS is recommended by Microsoft Azure IoT](https://azure.microsoft.com/en-us/blog/azure-iot-automatic-device-management-helps-deploying-firmware-updates-at-scale/)
- Texas Instruments - [an official partner of Texas Instruments](http://www.ti.com/ww/en/internet_of_things/iot-cloudsolution.html)
- STMicroelectronics - [an official partner of STMicroelectronics](https://www.st.com/content/st_com/en/partner/partner-program/partnerpage/cesanta.html)
- Espressif Systems - [an official partner of Espressif Systems](http://espressif.com/en/support/download/sdk)



# Docs, Support
- [Mongoose OS Documentation](https://mongoose-os.com/docs/mongoose-os/quickstart/setup.md)
- [Support Forum - ask your technical questions here](https://community.mongoose-os.com/)
- [Video tutorials](https://www.youtube.com/channel/UCZ9lQ7b-4bDbLOLpKwjpSAw/featured)
- [Commercial licensing](https://mongoose-os.com/licensing.html) and [support available](https://mongoose-os.com/support.html)

# Licensing

Mongoose OS is Open Source and dual-licensed:

- **Mongoose OS Community Edition** - Apache License Version 2.0
- **Mongoose OS Enterprise Edition** - Commercial License


## Community vs Enterprise Edition

|              |  Community Edition |  Enterprise Edition  |
| -------------| ------------------ | -------------------- |
| License | [Apache 2.0](https://www.apache.org/licenses/LICENSE-2.0) | Commercial - [contact us](https://mongoose-os.com/contact.html) |
| Allows to close end-product's source code  | Yes | Yes  |
| Price  | Free | Paid, see [details](https://mongoose-os.com//licensing.html) |
| Source code & functionality  | [Limited](https://mongoose-os.com/docs/mongoose-os/userguide/licensing.md) | Full |
| Technical support  | Community support via [Forum](https://forum.mongoose-os.com) and [Chat](https://gitter.im/cesanta/mongoose-os) | Commercial support by Mongoose OS development team, see [details](https://mongoose-os.com/support.html) |


# How to contribute

- If you have not done it already, sign [Cesanta CLA](https://cesanta.com/cla.html)
and send GitHub pull request.
- Make a Pull Request (PR) against this repo. Please follow
  [Google Coding Style](https://google.github.io/styleguide/cppguide.html).
  Send PR to one of the core team member:
   * [pimvanpelt](https://github.com/pimvanpelt)
   * [nliviu](https://github.com/nliviu)
   * [DrBomb](https://github.com/DrBomb)
   * [kzyapkov](https://github.com/kzyapkov)
   * [rojer](https://github.com/rojer)
   * [cpq](https://github.com/cpq)
- Responsibilities of the core team members:
   * Review and merge PR submissions
   * Create new repos in the https://github.com/mongoose-os-apps and
   https://github.com/mongoose-os-libs organisations for new app/library
   contributions
   * Create Mongoose OS releases
