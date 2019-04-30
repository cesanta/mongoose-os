# A device provision library

This library provides the following functionality:


## Reset to factory defaults on button press

This functionality allows to reset the device to factory defaults by
performing the following steps:

- remove `conf9.json` which holds user-specific device configuraiton
- perform filesystem garbage collection
- reboot the device

Reset to factory defaults is done by configuring a reset button, attached to a
certain GPIO pin. Two modes are possible:

- Hold the reset button and reboot the device while holding a button.
  For this, set `provision.btn.pin` to a non-negative value, and set 
  `provision.btn.hold_ms` to `0`.
- Just hold the reset button pressed for a long time.
  For this, set `provision.btn.pin` to a non-negative value, and set 
  `provision.btn.hold_ms` to a positive value, e.g. `5000` - 5 seconds.


## Reset to factory defaults on bad WiFi credentials

This functionality resets the device to factory defaults if the WiFi
configuration is entered by user incorrectly
(wrong network name or wrong password).

If the device has at least once connected to the WiFi station successfully,
reset is never done afterwards.

This is done via configuration parameter `provision.wifi_configured`, which is
set to `false` on a fresh device. When the device first connects to the WiFi
station, `provision.wifi_configured` is set to `true`. When WiFi connection
fails, and `provision.wifi_configured` is `false`, factory reset is triggered.


## Provisioning state

Device's provisioning state is tracked as two variables: current and maximum.
Both start at 0 but the current state resets on each boot and maximum state is persisted across reboots.

Maximum state can be used to ensure successful provisioning by specifying a
"stable state" (`provision.stable_state`) and a timeout (`provision.timeout`).
If stable state is not reached within the specified time, device config is wiped and device is rebooted
(which also resets max state to 0).

Four states are pre-defined:

 * Unprovisioned (0). Nothing is configured, will stay in this state without reboot no matter what stable state is configured.
 * Networking configured, connecting (1)
 * Connecting to cloud (2)
 * Connected to cloud (3)

Default value for `provision.stable_state` is 3, so the device will assume it has been fully provisioned once connected to the cloud.

If cloud connection is not required, this can be lowered to 2. If set to 0, device reset is not performed.

Stable state higher than 3 can be used if additional provisioning steps after successful connection are required.
User application code will need to manage further provisioning and use `mgos_provision_set_cur_state()` to signal state transitions.

When state transition occurs an event is raised which application code can use to indicate current provisioning state.

### LED indication

If `provision.led.pin` is configured, an event handler will be installed that will use LED to visually indicate the current provisioning state:

 * Unprovisioned (0): Blink the LED slowly (once a second)
 * Networking configured, connecting (1): Blink faster (every 0.5s)
 * Connecting to cloud (2): Blink fast (every 0.25s)
 * Connected to cloud (3, or whatever configured stable state is): Solid on

## Configuration parameters reference

```javascript
"provision": {
  "configured": false,  // Set to true when first time connected to WiFi
  "button": {
    "pin": 17,      // Reset button GPIO number. Negative value disables reset
                    // button functionality. Default: arch-specific.
    "hold_ms": 0,   // Number of milliseconds to hold to trigger a factory reset.
                    // If negative, disable. If 0, reset on boot.
                    // If positive, reset when pressed for that many milliseconds.
  }
}
```
