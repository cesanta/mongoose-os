---
title: "mgos_aws_shadow_state_handler"
decl_name: "mgos_aws_shadow_state_handler"
symbol_kind: "typedef"
signature: |
  typedef void (*mgos_aws_shadow_state_handler)(
      void *arg, enum mgos_aws_shadow_event ev, uint64_t version,
      const struct mg_str reported, const struct mg_str desired,
      const struct mg_str reported_md, const struct mg_str desired_md);
  typedef void (*mgos_aws_shadow_error_handler)(void *arg,
                                                enum mgos_aws_shadow_event ev,
                                                int code, const char *message);
---

Main AWS Device Shadow state callback handler.

Will get invoked when connection is established or when new versions
of the state arrive via one of the topics.

CONNECTED event comes with no state.

For DELTA events, state is passed as "desired", reported is not set. 

