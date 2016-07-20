---
title: Library initialisation
---

In order to make the development of client applications even easier, we maintain
Clubby and Cloud libraries in a number of languages.

For generated API documentation, please follow:

- [Generated JavaDoc for Cloud library](/cloud_java/latest). Cloud library
  includes the Clubby implementation, plus the helper stubs for accessing the
  cloud. More on that later.

Now that we have ID and PSK, we can create and initialise clubby instance:

```cs_examples_begin
```

```java
// Create a new clubby instance which will talk to default backend
// "//api.cesanta.com"
Clubby clubby = new Clubby.Builder()
    .id("my_id")    // Specify local address (e.g. device id)
    .psk("my_psk")  // Specify PSK
    .timeout(5)     // Specify default timeout for all requests
    .build();

// Possibly add a listener
clubby.addListener(
    new ClubbyAdapter() {
      @Override
      public void onConnected(Clubby clubby) {
        System.out.println("Clubby connected");
      }

      @Override
      public void onDisconnected(Clubby clubby) {
        System.out.println("Clubby disconnected");
      }

      // ... probably other methods of the ClubbyListener
    }
);

clubby.connect();
```

```javascript
// TODO (currently we don't have a separate JavaScript library)
```

```cs_examples_end
```

Having that done, we can use the `clubby` instance to talk to the cloud and other
devices.
