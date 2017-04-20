---
title: "Sys"
items:
---

System API. Source C API is defined at:
[mgos_hal.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_hal.h)



## **`Sys.calloc(nmemb, size)`**
Allocate a memory region.
Note: currently memory allocated this way must be explicitly released with `free()`.



## **`Sys.total_ram()`**
Return total available RAM in bytes.



## **`Sys.free_ram()`**
Return free available RAM in bytes.



## **`Sys.reboot()`**
Reboot the system. Return value: none.



# **`Sys.uptime()`**
Return number of seconds since last reboot.



## **`Sys.usleep(microseconds)`**
Sleep given number of microseconds.
Return value: none.

