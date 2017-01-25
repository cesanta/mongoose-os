let Net = {
  bind: ffi('void *mgos_bind(char *, void (*)(void *, int, void *, userdata), userdata)'),
  connect: ffi('void *mgos_connect(char *, void (*)(void *, int, void *, userdata), userdata)'),
  disconnect: ffi('void mgos_disconnect(void *)'),
  rsend: ffi('int mg_send(void *, char *, int)'),
  send: function(c, msg) { return Net.rsend(c, msg, msg.length); },
  EV_POLL: 0,
  EV_ACCEPT: 1,
  EV_CONNECT: 2,
  EV_RECV: 3,
  EV_SEND: 4,
  EV_CLOSE: 5,
  EV_TIMER: 6,
};
