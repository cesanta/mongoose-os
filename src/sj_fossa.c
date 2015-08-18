#include <fossa.h>

struct ns_mgr sj_mgr;

void init_fossa() {
  ns_mgr_init(&sj_mgr, NULL);
}

void destroy_fossa() {
  ns_mgr_free(&sj_mgr);
}

int poll_fossa() {
  if (ns_next(&sj_mgr, NULL) != NULL) {
    ns_mgr_poll(&sj_mgr, 1000);
    return 1;
  } else {
    return 0;
  }
}
