load('api_mqtt.js');
load('api_gpio.js');
load('api_sys.js');

let pin = 0, topic = 'my/topic';

GPIO.set_button_handler(pin, GPIO.PULL_UP, GPIO.INT_EDGE_NEG, 200, function() {
  let msg = {total_ram: Sys.total_ram(), free_ram: Sys.free_ram()};
  MQTT.pub(topic, JSON.stringify(msg), 1);
}, null);

MQTT.sub(topic, function(conn, topic, msg) {
  print('Topic:', topic, 'message:', msg);
}, null);
