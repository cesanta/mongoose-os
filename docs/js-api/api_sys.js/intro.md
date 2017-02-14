---
title: "Sys"
items:
---

 System API. Source C API is defined at:
 [mgos_hal.h](https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_hal.h)



 **`Sys.peek(ptr, offset)`** - return byte value (a number) for given
 pointer at offset.



 **`Sys.total_ram()`** - return total available RAM in bytes.



 **`Sys.free_ram()`** - return free available RAM in bytes.



 **`Sys.reboot()`** - reboot the system.
 Return value: none.



 **`Sys.usleep(microseconds)`** - sleep given number of microseconds.
 Return value: none.

