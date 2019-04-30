# Allows to call local RPC services - implements RPC Loopback channel

This RPC service provides a way to call local RPC services - i.e. those
running on the device itself.

Example usage in C/C++:

```c
#include "mg_rpc_channel_loopback.h"
struct mg_rpc_call_opts opts = {.dst = mg_mk_str(MGOS_RPC_LOOPBACK_ADDR) };
mg_rpc_callf(mgos_rpc_get_global(), mg_mk_str("My.Func"), NULL, NULL, &opts,
              "{param1: %Q, param2: %d}", "jaja", 1234);
```

Example usage in JavaScript:


```javascript
RPC.call(RPC.LOCAL, 'Config.Save', {reboot: true}, function (resp, ud) {
  print('Response:', JSON.stringify(resp));
}, null);
```
