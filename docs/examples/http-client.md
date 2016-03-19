---
title: HTTP client example
---

This example shows how to send periodic measurements to some external
wev server.

- Store this JavaScript snippet into a file `http_client_1.js`

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

- Upload file to the device: start FlashNChips, go to File/Upload, choose
  `http_client_1.js`
- In the FlashNChips console, connect to WiFi and execute the file

<img src="../../static/img/smartjs/http_client_1.png" align="center"/>

