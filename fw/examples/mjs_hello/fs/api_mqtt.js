// MQTT API. Source C API is defined at:
// https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_mqtt.h

let MQTT = {
  sub: ffi('void mgos_mqtt_sub(char *, void (*)(void *, char *, userdata), userdata)'),
  pub: ffi('int mgos_mqtt_pub(char *, char *, int)')
};
