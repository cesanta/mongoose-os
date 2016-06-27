---
title: "PubSub"
---

PubSub service.

#### Pull
Pull messages from a subscription.

Arguments:
- `subscription`: Name of the subscription.

Result `array`: 
Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/PubSub.Pull",
  "args": {
    "subscription": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123,
  "result": "VALUE PLACEHOLDER"
}

```

#### CreateTopic
Creates a new topic.

Arguments:
- `name`: Name of the topic.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/PubSub.CreateTopic",
  "args": {
    "name": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```

#### Ack
Acknowledge one or more messages.

Arguments:
- `ids`: List of message IDs to acknowledge.
- `subscription`: Name of the subscription.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/PubSub.Ack",
  "args": {
    "ids": "VALUE PLACEHOLDER",
    "subscription": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```

#### CreateSubscription
Creates a new subscription to a given topic. If messages are not acknowledged in `deadline` seconds they will be resent.

Arguments:
- `topic`: Name of the topic.
- `deadline`: Deadline in seconds for message delivery.
- `name`: Name of subscription.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/PubSub.CreateSubscription",
  "args": {
    "deadline": "VALUE PLACEHOLDER",
    "name": "VALUE PLACEHOLDER",
    "topic": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```

#### Publish
Publish a message to a topic.

Arguments:
- `topic`: Name of the topic.
- `data`: Message to publish.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/PubSub.Publish",
  "args": {
    "data": "VALUE PLACEHOLDER",
    "topic": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```

#### ExtendDeadline
Extend ack deadline for one or more messages. The new deadline is measured relative the this command. The deadline, measured in seconds, must be >= 0.

Arguments:
- `deadline`: Number of seconds since now for the new deadline.
- `ids`: List of message IDs to extend deadline for.
- `subscription`: Name of the subscription.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/PubSub.ExtendDeadline",
  "args": {
    "deadline": "VALUE PLACEHOLDER",
    "ids": "VALUE PLACEHOLDER",
    "subscription": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```

#### DeleteSubscription
Deletes a new subscription.

Arguments:
- `name`: Name of the subscription.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/PubSub.DeleteSubscription",
  "args": {
    "name": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```

#### DeleteTopic
Deletes a topic.

Arguments:
- `name`: Name of the topic.

Request:
```json
{
  "v": 2,
  "src": "device_123",
  "id": 123,
  "method": "/v1/PubSub.DeleteTopic",
  "args": {
    "name": "VALUE PLACEHOLDER"
  }
}

```

Response:
```json
{
  "v": 2,
  "src": "//api.mongoose-iot.com",
  "dst": "device_123",
  "id": 123
}

```


