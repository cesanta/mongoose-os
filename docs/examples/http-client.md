---
title: HTTP client
---

This example shows how to send periodic measurements to some external
wev server.

- Login to the cloud IDE
- Copy/paste the following code into the `app.js`

```javascript
function report() {
  print('sending request...');
  Http.request({
    hostname: 'httpbin.org',
    path: '/get?n=' + GC.stat().sysfree  // Free RAM
  }, function(response) { print(response); })
  .end( /* no POST data */ )
  .setTimeout(3000, function() {
    this.abort();   // Timeout request after 3 seconds
  });

  setTimeout(report, 3000);  // Report every 3 seconds
};

report();
```

- Make sure that the device is connected to the Internet
- Build and Flash firmware to the device
