---
title: Over-the-air (OTA) update
---

It's time to show how the OTA update works.

Let's build a new firmware. Put this code to the `src/app.c` that
prints a new startup message to the UART:

```c
#include <stdio.h>

#include "fw/src/mg_app.h"

enum mg_app_init_result mg_app_init(void) {
  printf("OTA WORKS!!\n");
  return MG_APP_INIT_SUCCESS;
}
```

Now build it:

```sh
$ miot cloud build
```

The new firmware is saved as `build/fw.zip`. Upload it to some server
of your choice.

Start a separate terminal window, and attach a console to the device
in order to see all messages that device prints to the UART:

```sh
$ miot dev console --port /dev/ttyUSB0
```

In the first terminal, trigger an OTA update:

```sh
$ miot cloud dev update https://mongoose-iot.com/downloads/tmp/fw.zip city=Dublin
Retrieving list of devices to update...
Start update for "d_JiFZ"
```

In the second terminal, we can see the that the new firmware has been loaded:

```
...
OTA WORKS!!
esp_mg_init          Init done, RAM: 37424 free
...
```
