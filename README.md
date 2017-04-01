# Mongoose OS

- [Mongoose OS](https://mongoose-os.com) - open source embedded operating system for low-power connected microcontrollers. 

Targeting commercial connected products:
- Code in C or JavaScript;
- security features like crypto chip support, filesystem encryption, etc; 
- built-in AWS IoT integration;
- [Embedded JavaScript engine - mJS](https://github.com/cesanta/mjs);
- available for ESP8266, ESP32, TI CC3200, STM32.

Mongoose OS benefits:
- Quick proof of concepts / prototypes;
- Shorter time to market at optimal cost;
- Secure, trusted and verified by leading companies;
- Reliable infrastructure for commercial products.

Trusted and Recommended By:
- Amazon AWS - [Amazon AWS Technology Partner](https://aws.amazon.com/partners/find/partnerdetails/?id=0010L00001jQCb5QAG)
- Texas Instruments - [an official partner of Texas Instruments](http://www.ti.com/ww/en/internet_of_things/iot-cloudsolution.html)
- Espressif Systems - [an official partner of Espressif Systems](http://espressif.com/en/ecosystem/cloud-platform)

# Docs, Support
- [Video tutorials](https://mongoose-os.com/#videos)
- [Mongoose OS Documentation](https://mongoose-os.com/docs/#/overview/)
- [Support Forum - ask your technical questions here] (http://forum.cesanta.com/index.php?p=/categories/mongoose-iot)
- [Commercial licensing and support available](https://mongoose-os.com/contact.html)

# Licensing

Mongoose OS is released under Commercial and [GNU GPL v.2](http://www.gnu.org/licenses/old-licenses/gpl-2.0.html) open source licenses.

Commercial Projects: [Contact us for the commercial license.] (https://www.cesanta.com/contact)

# Contributions

To submit contributions, sign
[Cesanta CLA](https://docs.cesanta.com/contributors_la.shtml)
and send GitHub pull request. You retain the copyright on your contributions.

## Building the mos tool for developers

### Dockerised builds

First, install Docker using the instructions from [download.docker.com](https://download.docker.com/).

**For PC or Mac**, simply do

```
docker build -t mos .
docker run -p 1992:1992 -v /dev/ttyUSB0:/dev/ttyUSB0 --privileged mos
```

If you are on a Mac, change the first ttyUSB0 to whatever your Mac calls your attached device (leave the second one alone).

**For Raspberry Pi and other ARM SBCs**, (Beagle Bone, Orange Pi, Nano Pi etc), do:

```
docker build -t golang-armhf -f Dockerfile-golang-armhf .
docker build -t mos -f Dockerfile-armhf .
docker run -p 1992:1992 -v /dev/ttyUSB0:/dev/ttyUSB0 --privileged mos
```

A Dockerised build allows you to run the Mongoose OS UI on a separate
system (such as a battery powered Raspberry Pi) giving you a shareable
and portable device programming station.

### Local builds

This presumes you are using Debian or Ubuntu.

**Install Go**

Install go using the instructions from [golang.org](http://golang.org).  (You could
also type `sudo apt install golang`, but you will get an older version of Go)

**Install dependencies**

```
apt install libftdi-dev python-git
go get -v github.com/kardianos/govendor
```

**Check out and build Mongoose OS**

```
git clone https://github.com/cesanta/mongoose-os/
cd mongoose-os
ln -s $PWD $GOPATH/src/cesanta.com
go get -d -v
govendor fetch github.com/jteeuwen/go-bindata/go-bindata
govendor fetch github.com/elazarl/go-bindata-assetfs/go-bindata-assetfs
make install
```

[![Analytics](https://ga-beacon.appspot.com/UA-42732794-6/project-page)](https://github.com/cesanta/mongoose-os)
