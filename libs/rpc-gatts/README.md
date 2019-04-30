# RPC over GATT, Server

## Overview

This library provides a GATT service that actes as an RPC channel.
It accepts incoming frames and can send them as well - or rather, make them available for collection.

*Note*: Default BT configuration is permissive. See https://github.com/cesanta/mongoose-os/libs/bt-common#security for a better idea.

## Attribute description

The service UUID is `5f6d4f53-5f52-5043-5f53-56435f49445f`, which is a representation of a 16-byte string `_mOS_RPC_SVC_ID_`.

Three attributes are defined:

 - `5f6d4f53-5f52-5043-5f64-6174615f5f5f (_mOS_RPC_data___)` - a r/w attribute used to submit frames for tx to the device and read out frames from the device.

 - `5f6d4f53-5f52-5043-5f74-785f63746c5f (_mOS_RPC_tx_ctl_)` - a write-only attribute. Before sending a frame expected length of the frame is submitted as a big-endian 32-bit number (so, for a 100 byte frame bytes `00 00 00 64` should be sent), followed by any number of writes to the data attribute. Chunking can be arbitrary, but the result must add up to the specified length exactly, at which point frame will be processed. Write to `tx_ctl` clears out any half-written frame that might be buffered, so writer needs to ensure there's only one frame in flight at any time.

 - `5f6d4f53-5f52-5043-5f72-785f63746c5f (_mOS_RPC_rx_ctl_)` - a read/notify attribute. It returns the length of the frame that device wishes to transmit as a big-endian 32-bit number. If this value is not zero, frame data will be returned in response to read requests of the data attribute. Read chunks will be up to MTU bytes in size. Client may subscribe to notifications on this attribute. Notification will be sent whenever a new frame is submitted for delivery and the notification body will contain length (the same value as returned by reading). Upon receiving notification client can proceed to read the data without reading `rx_ctl` again.
