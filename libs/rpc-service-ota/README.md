# RPC Service - OTA (Over The Air updates)

This service provides an ability to manage OTA on devices remotely.
It is possible to call this service programmatically via serial, HTTP/RESTful,
Websocket, MQTT or other transports
(see [RPC section](/docs/mongoose-os/userguide/rpc.md)) or via the `mos` tool.

See in-depth description of our OTA mechanism at
[Updating firmware reliably - embedded.com](http://www.embedded.com/design/prototyping-and-development/4443082/Updating-firmware-reliably).

See OTA video tutorial:

<iframe width="560" src="https://www.youtube.com/embed/o05sBDfaFO8"
  height="315" align="center" frameborder="0" allowfullscreen></iframe>

Below is a list of exported RPC methods and arguments:

## OTA.Update
Trigger OTA firmware update. Arguments:
```javascript
{
  "url": "https://foo.com/fw123.zip", // Required. URL to the new firmware.
  "commit_timeout": "300"             // Optional. Time frame in seconds to do
                                      // OTA.Commit after reboot. If commit is
                                      // not done during the timeout, OTA rolls back.
}
```

A new firmware gets downloaded to the separate flash partition,
and is marked dirty. When the download is complete, device is rebooted.
After reboot, a firmware partition could become committed by calling
`OTA.Commit` - in which case, it is marked as "good". Otherwise, a device
reboots back into the old firmware after the `commit_timeout` seconds.
Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call OTA.Update '{"url": "http://1.2.3.4/fw.zip", "commit_timeout": 300}'</code></pre>


## OTA.Commit
Commit current firmware. Arguments: none.

Example usage:

<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call OTA.Commit</code></pre>


## OTA.Revert
Rolls back to the previous firmware. Arguments: none.

Example usage:
<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call OTA.Revert</code></pre>


## OTA.CreateSnapshot
Create new firmware patition with the copy of currently running firmware. Arguments:
```javascript
{
  // Optional. If true, then current firmware is uncommited, and needs to
  // be explicitly commited after the first reboot. Otherwise, it'll reboot
  // into the created snapshot. This option is useful if a dangerous, risky
  // live update is to be done on the living device. Then, if the update
  // fails and device bricks, it'll revert to the created good snapshot.
  "set_as_revert": false,
  // Optional. Same meaning as for OTA.Update
  "commit_timeout": "300"
}
```

Example usage:
<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call OTA.CreateSnapshot</code></pre>


## OTA.GetBootState
Get current boot state. Arguments: none.

Example usage:
<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call OTA.GetBootState
{
  "active_slot": 0,       # Currently active flash partition.
  "is_committed": true,   # Current firmware is marked as "good" (committed).
  "revert_slot": 0,       # If uncommitted, slot to roll back to.
  "commit_timeout": 0     # Commit timeout.
}</code></pre>

## OTA.SetBootState
Get current boot state. Arguments: see `OTA.GetBootState` reply section.

Example usage:
<pre class="command-line language-bash" data-user="chris" data-host="localhost" data-output="2-100"><code>mos call OTA.SetBootState '{"revert_slot": 1}'</code></pre>
