---
title: "mgos_mqtt_pub()"
decl_name: "mgos_mqtt_pub"
symbol_kind: "func"
signature: |
  bool mgos_mqtt_pub(const char *topic, const void *message, size_t len, int qos);
---

Publish message to the configured MQTT server, to the given MQTT topic.
Return value will be true if there is a connection to the server and the
message has been queued for sending. In case of QoS 1 return value does
not indicate that PUBACK has been received; there is currently no way to
check for that. 

