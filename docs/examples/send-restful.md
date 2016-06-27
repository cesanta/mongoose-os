---
title: Send data to a RESTful server
---

This example shows how to send periodic measurements to some external
RESTful server. This example uses http://httpbin.org, which is a useful public
RESTful service that allows to test or debug RESTful interfaces.

Every 5 seconds we measure free RAM, and send http://httpbin.org/get?n=NUMBER
request, where `NUMBER` is a measured number. httpbin.org echoes back a
JSON object that describes the request. We simply log this received reply.
The log gets sent to the Mongoose Cloud, and is visible on a device console.


- Login to [Mongoose Cloud](https://mongoose-iot.com)
- Create a new project, call it `restful`
- Swith to the IDE tab
- Copy/paste the following code into the `app.js`

    ```javascript
    console.log('Hello from restful example');

    function report() {
      Http.request({
        hostname: 'httpbin.org',
        path: '/get?n=' + GC.stat().sysfree
      }, function(response) { console.log(response); })
      .end( /* no POST data */ )
      .setTimeout(3000, function() {
        this.abort();   // Timeout request after 3 seconds
      });

      setTimeout(report, 5000);
    };

    report();
    ```

- In the IDC (Interactive Device Console), choose your target device
- Click Flash button, and wait until messages start to appear
