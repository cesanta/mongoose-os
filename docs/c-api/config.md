---
title: Configuration
---

The system configuration object is loaded in RAM at firmware boot time and
stays there. It is accessible via the `get_cfg()` call.

To add application-specific config parameters, create a
`fs/conf_app_defaults.json` file, for example:

```json
{
  "hello": {
    "who": "world"
  }
}
```

Then, C code can access it this way:

```c
  printf("Hello, %s!\n", get_cfg()->hello.who);
```

See [c_hello example](https://github.com/cesanta/mongoose-iot/blob/master/fw/examples/c_hello/src/app_main.c)
for an example usage.
