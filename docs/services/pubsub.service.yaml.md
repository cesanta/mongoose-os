---
title: "PubSub service"
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
  "v": 1,
  "src": "//api.cesanta.com/device_123",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/PubSub.Pull",
      "id": 123,
      "args": {
        "subscription": "sub1"
      }
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com",
  "dst": "//api.cesanta.com/device_123",
  "resp": [
    {
      "id": 123,
      "status": 0,
      "resp": [
        {
          "ackID": "msg123",
          "message": {
            "data": "hello world!"
          }
        }
      ]
    }
  ]
}

```

#### CreateTopic
Creates a new topic.

Arguments:
- `name`: Name of the topic.

Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/device_123",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/PubSub.CreateTopic",
      "id": 123,
      "args": {
        "name": "topic1"
      }
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com",
  "dst": "//api.cesanta.com/device_123",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
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
  "v": 1,
  "src": "//api.cesanta.com/device_123",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/PubSub.Ack",
      "id": 123,
      "args": {
        "ids": "msg123",
        "subscription": "sub1"
      }
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com",
  "dst": "//api.cesanta.com/device_123",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
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
  "v": 1,
  "src": "//api.cesanta.com/device_123",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/PubSub.CreateSubscription",
      "id": 123,
      "args": {
        "deadline": 300,
        "name": "sub1",
        "topic": "topic1"
      }
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com",
  "dst": "//api.cesanta.com/device_123",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
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
  "v": 1,
  "src": "//api.cesanta.com/device_123",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/PubSub.Publish",
      "id": 123,
      "args": {
        "data": "hello world!",
        "topic": "topic1"
      }
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com",
  "dst": "//api.cesanta.com/device_123",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
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
  "v": 1,
  "src": "//api.cesanta.com/device_123",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/PubSub.ExtendDeadline",
      "id": 123,
      "args": {
        "deadline": 300,
        "ids": ["msg123"],
        "subscription": "sub1"
      }
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com",
  "dst": "//api.cesanta.com/device_123",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
}
```

#### DeleteSubscription
Deletes a new subscription.

Arguments:
- `name`: Name of the subscription.

Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/device_123",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/PubSub.DeleteSubscription",
      "id": 123,
      "args": {
        "name": "sub1"
      }
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com",
  "dst": "//api.cesanta.com/device_123",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
}
```

#### DeleteTopic
Deletes a topic.

Arguments:
- `name`: Name of the topic.

Request:
```json
{
  "v": 1,
  "src": "//api.cesanta.com/device_123",
  "dst": "//api.cesanta.com",
  "cmds": [
    {
      "cmd": "/v1/PubSub.DeleteTopic",
      "id": 123,
      "args": {
        "name": "topic1"
      }
    }
  ]
}

```

Response:
```json
{
  "v": 1,
  "src": "//api.cesanta.com",
  "dst": "//api.cesanta.com/device_123",
  "resp": [
    {
      "id": 123,
      "status": 0
    }
  ]
}
```


