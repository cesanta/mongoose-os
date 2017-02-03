---
title: Overview
---

Often, IoT boards provide no built-in flash protection mechanism.
Anyone with a physical access to the device can read the whole flash,
including any sensitive information like TLS private keys.

Crypto chips are designed to mitigate that.
Their main function is provide storage for private keys, which cannot be read.
Private keys are stored inside the crypto chip, and all the crypto operations
that require private key, are offloaded to the crypto chip which performs
the operation and gives the result back. 

[ATECC508A crypto chip](http://www.atmel.com/devices/ATECC508A.aspx)
is designed with additional hardware protection mechanisms
to make key extraction difficult. It is an impressive piece of hardware with
many layers of protection, and important enough it is quite inexpensive,
costing less than 80 cent a piece.
