---
title: Custom messages
---

Other than talking to the cloud, we are also likely to want to send some custom
messages to our devices we want to control.

As a simple example, assume we want to control GPIO state of pins on the remote
device. Assume the device is running Mongoose IoT firmware, and it has the
following code in its `app.js`:

```javascript
clubby.oncmd(
  "/v1/GPIO.Read",
  function(data){
    var args = data.args;
    return {
      pin: args.pin,
      state: GPIO.read(args.pin),
    };
  }
);

clubby.oncmd(
  "/v1/GPIO.Write",
  function(data){
    var args = data.args;
    GPIO.write(args.pin, args.state);
    return {
      pin: args.pin,
      state: GPIO.read(args.pin),
    };
  }
);

```

So, it responds on two commands: `/v1/GPIO.Read` and `/v1/GPIO.Write`. Now we
want to control this device remotely.

Let's define the appropriate data structures:


```cs_examples_begin
```

```java
/**
 * Arguments for our custom command "/v1/GPIO.Read"
 */
static class GpioReadArgs {
    public int pin;

    GpioReadArgs(int pin) {
        this.pin = pin;
    }
}

/**
 * Arguments for our custom command "/v1/GPIO.Write"
 */
static class GpioWriteArgs {
    public int pin;
    public boolean state;

    GpioWriteArgs(int pin, boolean state) {
        this.pin = pin;
        this.state = state;
    }
}

/**
 * Response of our custom commands "/v1/GPIO.Read" and "/v1/GPIO.Write",
 * will be deserialized from JSON
 */
static class GpioResp {
    public int pin;
    public int state;
}
```

```javascript
// TODO
```

```cs_examples_end
```

And now, we can perform requests. Here we assume that the device we send
commands to has the id `"//api.cesanta.com/d/mydevice"`. Let's read the
state of the 4th pin:


```cs_examples_begin
```

```java
int pinNumber = 4;
clubby.call(
        "//api.cesanta.com/d/mydevice",
        "/v1/GPIO.Read",
        new GpioReadArgs(pinNumber),
        new CmdAdapter&lt;GpioResp&gt; {
            @Override
            public void onResponse(final GpioResp resp) {
                if (resp != null) {
                    switch (resp.state) {
                        case 0:
                            System.out.println("Pin state: OFF");
                            break;
                        case 1:
                            System.out.println("Pin state: ON");
                            break;
                        default:
                            System.err.println("Invalid pin number");
                            break;
                    }
                }
            }

            @Override
            public void onError(int status, String status_msg) {
                System.err.println("Got error: " + status_msg);
            }
        },
        GpioResp.class
    );
```

```javascript
// TODO
```

```cs_examples_end
```

And let's set the state of the pin to "ON":

```cs_examples_begin
```

```java
int pinNumber = 4;
clubby.call(
        "//api.cesanta.com/d/mydevice",
        "/v1/GPIO.Write",
        new GpioWriteArgs(pinNumber, true),
        CmdAdapter&lt;GpioResp&gt; {
            @Override
            public void onResponse(final GpioResp resp) {
                if (resp != null) {
                    switch (resp.state) {
                        case 0:
                            System.out.println("Pin state: OFF");
                            break;
                        case 1:
                            System.out.println("Pin state: ON");
                            break;
                        default:
                            System.err.println("Invalid pin number");
                            break;
                    }
                }
            }

            @Override
            public void onError(int status, String status_msg) {
                System.err.println("Got error: " + status_msg);
            }
        },
        GpioResp.class
    );
```

```javascript
// TODO
```

```cs_examples_end
```

This is it! You can easily send any custom commands you need, and it's even
easier to talk to the cloud thanks to the helper classes. For exact API of
these helpers, consult the API documentation of the library in the language of
interest.

