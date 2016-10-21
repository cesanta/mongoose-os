#include "fw/src/mg_console_js.h"

#if MG_ENABLE_JS

#include <stdlib.h>
#include <string.h>

#include "fw/src/mg_common.h"
#include "fw/src/mg_console.h"

#include "v7/v7.h"

static void mg_console_puts_n(const char *s, int len) {
  while (len-- > 0) {
    mg_console_putc(*s);
    s++;
  }
}

MG_PRIVATE enum v7_err Console_log(struct v7 *v7, v7_val_t *res) {
  int argc = v7_argc(v7);
  /* Put everything into one message */
  for (int i = 0; i < argc; i++) {
    v7_val_t arg = v7_arg(v7, i);
    if (v7_is_string(arg)) {
      size_t len;
      const char *str = v7_get_string(v7, &arg, &len);
      mg_console_puts_n(str, len);
    } else {
      char buf[100], *p;
      p = v7_stringify(v7, arg, buf, sizeof(buf), V7_STRINGIFY_DEBUG);
      mg_console_puts_n(p, strlen(p));
      if (p != buf) free(p);
    }
    if (i != argc - 1) {
      mg_console_putc(' ');
    }
  }
  mg_console_putc('\n');

  *res = V7_UNDEFINED; /* like JS print */
  return V7_OK;
}

void mg_console_api_setup(struct v7 *v7) {
  v7_val_t console_v = v7_mk_object(v7);
  v7_own(v7, &console_v);

  v7_set_method(v7, console_v, "log", Console_log);
  v7_set(v7, v7_get_global(v7), "console", ~0, console_v);

  v7_disown(v7, &console_v);
}

void mg_console_js_init(struct v7 *v7) {
  (void) v7;
}
#endif /* MG_ENABLE_JS */
