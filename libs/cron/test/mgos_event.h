enum mgos_event_sys {
  MGOS_EVENT_TIME_CHANGED,
};

typedef void (*mgos_event_handler_t)(int ev, void *ev_data, void *userdata);
static inline bool mgos_event_add_handler(int ev, mgos_event_handler_t cb,
                                          void *userdata) {
  (void) ev;
  (void) cb;
  (void) userdata;
}
