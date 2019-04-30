# RPC support for Google Cloud Platform

## Device support

This library rovides RPC support over the mechanisms provided by the Google IoT COre platform

 * Requests should be sent to the device as [commands](https://cloud.google.com/iot/docs/how-tos/commands).
   This library listens to commands on a particular subfolder (default: `rpc`) and parses the payload as RPC frames.

 * Responses are published as telemetry events, to the subfolder that is equal to the `dst` of the response (and thus `src` in the original request).
   So, if the caller wishes to receive response on subfolder `foo`, they should specify `src: \"foo\"` in their request.
   It should be noted that responses from all the devices that use the same response subfolder end up in the same topic and subscription.
   All the subscribers will receive messages posted here by all the devices. Subscribers need to take care to not ack messages that are not intended for them (match request ID).
   While acceptable for light use, this will cause unnecessary churn if multiple devices are communicating at the same time.
   In this case users should set up use multiple different response subfolders.

To receive responses, caller must set up the necessary PubSub topics and/ subscription and ensure that the desired subfolder is plumbed to the correct topic.
See [here](https://cloud.google.com/iot/docs/how-tos/mqtt-bridge#publishing_telemetry_events_to_separate_pubsub_topics) for detailed description.

## Mos tool support

`mos` supports GCP RPC mechanism through the `gcp` "port" type:

```
$ mos --port gcp://project/region/registry/device call Sys.GetInfo
{
  "app": "demo-c",
  "fw_version": "1.0",
  "fw_id": "20190123-121047/2.10.0-244-g81233d5ed-dirty-mos8",
  "mac": "1AFE34A5930F",
  "arch": "esp8266",
  "uptime": 5248,
  "ram_size": 51864,
  "ram_free": 37072,
  "ram_min_free": 21928,
  "fs_size": 233681,
  "fs_free": 153612,
  "wifi": {
    "sta_ip": "192.168.11.25",
    "ap_ip": "",
    "status": "got ip",
    "ssid": "Over_9000_Internets"
  }
}
```

Use your own `project`, `region`, `registry`, and `device`.
By default, `mos` uses `rpc` subfolder for telemetry events and subscribes to the `rpc` topic.
You will need to create the topic and forward the `rpc` subfolder to it.
Or allow `mos` to set up all the necessary plumbing byt adding `--gcp-create-topic` (Note: config changes to GCP take some time to propagate, so the first reuqest may fail).

You can customize subfolder names via query parameters (e.g. `mos --port=gcp://project/region/registry/device?sub=rpc1&respsf=rpc1`):

 * `sub` - subscription name. Default is `rpc`.
 * `topic` - topic name. Default is `rpc`.
 * `reqsf` - request subfolder. This needs to match `rpc.gcp.subfolder` on the device. Default is `rpc`.
 * `respsf` - response subfolder. Default is `rpc`.
   * As described above, there is inherent contention between subscribers when the same response subfolder is used for responses to requests by multiple callers.
     If the volume of churn (Nacks) becomes a problem, and until Google implements [this](https://googlecloudplatform.uservoice.com/forums/302631-cloud-pub-sub/suggestions/32439010-subscriber-message-filtering) or [this](https://googlecloudplatform.uservoice.com/forums/302631-cloud-pub-sub/suggestions/8668309-subscribe-to-topic-attributes), the workaround is to set up several response subfolders and shard responses between them.
