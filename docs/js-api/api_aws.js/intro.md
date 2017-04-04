---
title: "AWS"
items:
---

AWS API, see AWS documentation at
http://docs.aws.amazon.com/iot/latest/developerguide/iot-thing-shadows.html



## **`AWS.Shadow.setStateHandler(callback, userdata)`**
Set AWS shadow
state handler callback. When AWS shadow state changes, the callback is
called with the following arguments: `(userdata, event, reported,
desired)`, where `userdata` is the userdata given to `setStateHandler`,
`event` is one of the following: `AWS.Shadow.CONNECTED`,
`AWS.Shadow.GET_ACCEPTED`,
`AWS.Shadow.GET_REJECTED`, `AWS.Shadow.UPDATE_ACCEPTED`,
`AWS.Shadow.UPDATE_REJECTED`, `AWS.Shadow.UPDATE_DELTA`.
`reported` is previously reported state object (if any), and `desired`
is the desired state (if present).

Example:
```javascript
load('api_aws.js');

let state = { on: false, counter: 0 };

AWS.Shadow.setStateHandler(function(data, event, reported, desired) {
  print(JSON.stringify(reported), JSON.stringify(desired));
  if (event === AWS.Shadow.CONNECTED) {
    AWS.Shadow.update(0, {state: state});  // Report current state
  } else if (event === AWS.Shadow.UPDATE_DELTA) {
    // Cloud wants us to update state, apply changes
    for (let key in state) {
      if (desired.state[key] !== undefined) state[key] = desired[key];
    }
  }
}, null);
```



## **`AWS.Shadow.update(version, state)`**
Update AWS shadow state.
State should be an object with "reported" and/or "desired" keys.

Response will arrive via `UPDATE_ACCEPTED` or `UPDATE_REJECTED` events.
If you want the update to be aplied only if a particular version is
current, specify the version. Otherwise set it to 0 to apply to any
version. Example: increase `state.counter` on a button press:
```javascript
load('api_aws.js');
load('api_gpio.js');

let state = { on: false, counter: 0 };
let buttonPin = 0;

GPIO.set_button_handler(buttonPin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 200, function() {
  AWS.Shadow.update(0, {desired: {on: state.on, counter: state.counter + 1}});
}, null);
```

