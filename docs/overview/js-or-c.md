---
title: Code in C/C++ or JS
---

Device logic on Mongoose Firmware can be implemented using
traditional C/C++ ([API reference](#/c-api/)) or be scripted in
JavaScript ([API reference](#/javascript-api/))
thanks to the world's smallest
[JavaScript Engine - V7](https://github.com/cesanta/v7).

Code in C:

```c
#include <stdio.h>

#include "common/platform.h"
#include "fw/src/miot_app.h"
#include "fw/src/miot_sys_config.h"

enum miot_app_init_result miot_app_init(void) {
  printf("Hello, %s!\n", get_cfg()->hello.who);  // Print config value

  return MIOT_APP_INIT_SUCCESS;   // Signal successful initialization
}
```

Or in JavaScript:

```javascript
// app.js
var pin = 2;
var value = GPIO.read(pin);
console.log('Pin ' + pin + ' value: ' + value);
```

Or, use both!
