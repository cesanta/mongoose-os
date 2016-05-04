---
title: Library API
---

First of all, we need to create and initialize clubby instance:

```cs_examples_begin
```

```java
// Create a new clubby instance which will talk to default backend
// "//api.cesanta.com"
Clubby clubby = new Clubby.Builder()
    .device("my_id", "my_psk")  // Specify ID and PSK of the device
    .timeout(5)                 // Specify default timeout for all requests
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

Now, we can use `clubby` instance to talk to the cloud and other devices.

Let's say we want to request the cloud about all projects which the current
device has access to. For that, we have the
<a href="#/services/project.service.yaml/">Project service</a>.

So, let's create a ProjectService helper instance:

```cs_examples_begin
```

```java
ProjectService project = ProjectService.createInstance(clubby);
```

```javascript
// In JavaScript, no need for any separate entity, we should use the clubby
// instance directly
```

```cs_examples_end
```

And now, we can list all available projects as follows:

```cs_examples_begin
```

```java
project.list(
        new ProjectService.ListArgs(),
        new CmdAdapter<ProjectService.ListResponse>() {
            @Override
            public void onResponse(ProjectService.ListResponse resp) {

                // Got some response from the cloud.
                // 
                // ProjectService.ListResponse is an
                // ArrayList<ProjectService.ListResponseItem>,
                // so we can iterate through it as follows:

                for (ProjectService.ListResponseItem item : resp) {
                    // Just print project's id and name
                    System.out.println("id:" + item.id);
                    System.out.println("name:" + item.name);
                }
            }

            @Override
            public void onError(int status, String status_msg) {
                System.err.println("Got error: " + status_msg);
            }
        }
    );
```

```javascript
clubby.call(
  "//api.cesanta.com",
  {
    "cmd": "/v1/Project.List",
    "timeout": 5
  },
  function(obj) {
    if (obj.status == 0) {
      // Got positive response, let's just print it
      console.log(obj.resp);
    } else {
      console.error(obj.status_msg);
    }
  }
)
```

```cs_examples_end
```
