V7: Embedded JavaScript engine
==============================

[![Circle CI](https://circleci.com/gh/cesanta/v7.svg?style=shield)](https://circleci.com/gh/cesanta/v7)
[![License](https://img.shields.io/badge/license-GPL_2-green.svg)](https://github.com/cesanta/v7/blob/master/LICENSE)

V7 is a JavaScript engine written in C.
It makes it possible to program Internet of Things (IoT) devices
in JavaScript. V7 features are:

- Cross-platform: works on anything, starting from Arduino to MS Windows
- Small size. Compiled core is in 40 KB - 200 KB range
- Simple and intuitive C/C++ API. It is easy to export existing C/C++
  functions into JavaScript environment
- Standard: V7 aims to implement JavaScript 5.1 and pass standard ECMA tests
- Performance: V7 aims to be the fastest non-JIT engine available
- Usable out-of-the-box: V7 provides an auxiliary library with
  Hardware (SPI, UART, etc), File, Crypto, Network API
- Source code is both ISO C and ISO C++ compliant
- Very easy to integrate: simply copy two files: [v7.h](v7.h)
   and [v7.c](v7.c) into your project

## Examples & Documentation

- [User Guide](http://cesanta.com/docs/v7) - Detailed User Guide and API reference
- [examples](examples) - Collection of well-commented examples

# Contributions

People who have agreed to the
[Cesanta CLA](http://cesanta.com/contributors_la.html)
can make contributions. Note that the CLA isn't a copyright
_assigment_ but rather a copyright _license_.
You retain the copyright on your contributions.

## Licensing

V7 is released under commercial and
[GNU GPL v.2](http://www.gnu.org/licenses/old-licenses/gpl-2.0.html) open
source licenses. The GPLv2 open source License does not generally permit
incorporating this software into non-open source programs.
For those customers who do not wish to comply with the GPLv2 open
source license requirements,
[Cesanta](http://cesanta.com) offers a full,
royalty-free commercial license and professional support
without any of the GPL restrictions.
