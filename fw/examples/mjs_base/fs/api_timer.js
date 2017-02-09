// Timer API. Source C API is defined at:
// https://github.com/cesanta/mongoose-os/blob/master/fw/src/mgos_timers.h

let Timer = {
  set: ffi('int mgos_set_timer(int,int,void(*)(userdata),userdata)')
};