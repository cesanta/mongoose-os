#include <fossa.h>

struct ns_mgr sj_mgr;

void fossa_init() {
  ns_mgr_init(&sj_mgr, NULL);
}

void fossa_destroy() {
  ns_mgr_free(&sj_mgr);
}

int fossa_poll(int ms) {
  if (ns_next(&sj_mgr, NULL) != NULL) {
    ns_mgr_poll(&sj_mgr, ms);
    return 1;
  } else {
    return 0;
  }
}
