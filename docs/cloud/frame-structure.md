---
title: Clubby frame format
---

Mongoose IoT provides framing format called Clubby that makes
IoT communication simple and secure. Clubby is very similar to JSON-RPC.
It defines a format for requests and responses. Requests and responses are
encoded as human-readable [JSON](http://www.json.org/)
or size-efficient [Universal Binary JSON](http://ubjson.org/) frames.

Clubby frames can be carried by any transport protocol, like raw TCP, UDP,
HTTP, Websocket, MQTT, and so on. At this moment, Mongoose Cloud listens
on HTTPS and secure Websocket (WSS) ports.

Every Clubby frame has source, destination, and frame ID. Frame ID is used
to match requests with responses, as responses may arrive out-of-order.
All Clubby requests have `method` field which tells which method is called,
together with `args` field that specifies method's arguments. All Clubby
responses have `result` field, and `error` field in case if method call
failed.

Here is an example of JSON-ecoded Clubby request:

```json
{
  "src": "joe/32efc823aa",
  "id": 1772,
  "method": "/v1/Timeseries.Report",
  "args": {
    "name": "temperature",
    "value": 36.6
  }
}
```

A corresponding response:

```json
{
  "dst": "joe/32efc823aa",
  "id": 1772,
  "result": "ok"
}

```

Each Clubby frame (request or response) has following fields:

- `v`: optional frame format version, must be set to 2.
- `src`: optional source address, a string (see Clubby addresses below).
- `dst`: optional destination address, a string (see Clubby addresses below).
- `key`: optional pre-shared string key (sometimes called API key) used to
  verify that the sender is really who he claims to be.
- `id`: optional ID of the request or response.
  Positive integer, up to 2^53-1.
  Automatically generated if you don't
  provide one. Used to match responses with requests,
  as responses might come out-of-order.

Requests-specific fields:
- `method`: required method name, a string.
- `args`: optional JSON object representing method arguments.
  Type and format are method-dependent.
<br>Optional timeout can be specified in two ways:
- `deadline`: as an absolute UNIX timestamp, a positive integer, OR
- `timeout`: as a number of seconds, a positive integer

Response-specific fields:
- `result`: optional JSON object, a result of the method invocation.
  Type and format depend on the method.
- `error`: optional error JSON object. If this key is present,
  method call has failed. Error object has following format:
  - `code`: required numeric error code, != 0
  - `message`: required error message

If a response frame has an `error` field, method call
has failed. Otherwise, it succeeded.
