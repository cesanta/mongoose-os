---
title: "Cfg"
items:
---



## **`Cfg.get(path)`** 
Get the config value by the configuration variable. Currently, only
simple types are returned: strings, ints, booleans, doubles. Objects
are not yet supported.

Examples:
```javascript
load('api_config.js');
Cfg.get('device.id');        // returns a string
Cfg.get('debug.level');      // returns an integer
Cfg.get('wifi.sta.enable');  // returns a boolean
```

