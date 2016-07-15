---
title: Send data to Mongoose Cloud
---

This example shows you how to send data from a device to
[Mongoose Cloud](https://mongoose-iot.com).

Mongoose Cloud has three kinds of databases:
1. Accounts/Devices/Projects database: Keeps information about registered
   users, devices, projects, workspaces, etcetera.
2. Timeseries database: Used to keep historical telemetry values. For example,
   if devices are required to report any sort of sensor data, Timeseries
   database is an ideal place to keep that data. This dababase is backed by the
   popular [Prometheus Monitoring Solution](https://prometheus.io/).
   The Timeseries API also provides analytics functionality by means of the powerful
   query language, allowing you to build rich monitoring dashboards, etc.
3. Blob Store database: This is a key-value store where keys have structure,
   just like file system paths. You can think of Blob Store as an online
   filesystem. You can keep any sort of data there such as configuration files, etc.

This tutorial shows you how to report data to the Timeseries database and how
to retrieve stored historical values programmatically. This is a basic
building block for data analytics, monitoring dashboards and so on.

We are sending an amount of free RAM every 5 seconds:

- Login to [Mongoose Cloud](https://mongoose-iot.com).
- Create a new project, call it `timeseries`.
- Swith to the IDE tab.
- Copy/paste the following code into the `app.js`

    ```javascript
    console.log('Hello from timeseries example');
    var report = function() {
      clubby.call('/v1/Timeseries.Report', 'memory', GC.stat().sysfree);
      setTimeout(report, 5000);
    };
    clubby.ready(function() {
      report();
    });
    ```

- In the IDC (Interactive Device Console), choose your target device.
- Click the Flash button and wait for hello message to appear in the device log.
- From that point on, the device sends data to the cloud every 5 seconds.
  Now, let's fetch this data via the secure HTTP request:
- Open a new browser tab with the account information.
- Copy your Mongoose Cloud authentication token to the clipboard.
- Open terminal and enter the following `curl` command:

    ```sh
      curl -d '{
        "src":"YOUR_LOGIN",
        "key": "YOUR_AUTH_TOKEN",
        "method": "/v1/Timeseries.Query",
        "args": {"src": "YOUR_DEVICE_ID"}
      }' https://api.mongoose-iot.com
    ```

- The Curl output shows historical data. You can fetch this data using other
  languages, like JavaScript, Python, Java, etc.
