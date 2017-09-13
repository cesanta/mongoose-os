---
title: "mgos_mqtt_global_subscribe()"
decl_name: "mgos_mqtt_global_subscribe"
symbol_kind: "func"
signature: |
  void mgos_mqtt_global_subscribe(const struct mg_str topic,
                                  mg_event_handler_t handler, void *ud);
---

Subscribe to a specific topic.
This handler will receive SUBACK - when first subscribed to the topic,
PUBLISH - for messages published to this topic, PUBACK - acks for PUBLISH
requests. MG_EV_CLOSE - when connection is closed. 

