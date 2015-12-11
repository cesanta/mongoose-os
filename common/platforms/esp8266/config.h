#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include "sys_config.h"

/* Read-only firmware setting */
struct ro_var {
  struct ro_var *next;
  const char *name;
  const char **ptr;
};
extern struct ro_var *g_ro_vars;

#define REGISTER_RO_VAR(_name, _ptr) \
  do {                               \
    static struct ro_var v;          \
    v.name = (#_name);               \
    v.ptr = (_ptr);                  \
    v.next = g_ro_vars;              \
    g_ro_vars = &v;                  \
  } while (0)

int load_config(const char *defaults_config_file,
                const char *overrides_config_file,
                struct sys_config *config_struct);

int apply_config(const struct sys_config *config_struct);

#endif
