#include <mongoose.h>

struct mg_mgr sj_mgr;

void mongoose_init() {
  mg_mgr_init(&sj_mgr, NULL);
}

void mongoose_destroy() {
  mg_mgr_free(&sj_mgr);
}

int mongoose_poll(int ms) {
  if (mg_next(&sj_mgr, NULL) != NULL) {
    mg_mgr_poll(&sj_mgr, ms);
    return 1;
  } else {
    return 0;
  }
}
