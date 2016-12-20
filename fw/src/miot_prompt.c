/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */

#include "fw/src/miot_prompt.h"

#if MIOT_ENABLE_JS

#include "fw/src/miot_hal.h"
#include "fw/src/miot_v7_ext.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "common/mbuf.h"
#include "common/platform.h"
#include "v7/v7.h"

#include "fw/src/miot_rpc_channel_uart.h"
#include "fw/src/miot_rpc.h"
#include "fw/src/miot_uart.h"
#include "fw/src/miot_sys_config.h"
#include "fw/src/miot_v7_ext.h"

#define SIGINT_CHAR 0x03
#define EOF_CHAR 0x04

typedef void (*char_processor_t)(char ch);

struct miot_prompt_state {
  struct v7 *v7;
  char_processor_t char_processor;
  struct mbuf buf;
  int uart_no;
  int swallow;
  bool running;
};

static struct miot_prompt_state s_sjp;

static void process_js(const char *cmd);
static void process_prompt_char(char ch);

static void show_prompt(void) {
  /*
   * MFT relies on prompt ending with "$ " to detect when it's okay
   * to send the next line during file upload.
   */

  /* TODO RTOS(alashkin): RTOS printf doesn't support %lu */
  printf("[%u/%d] $ ", (unsigned int) miot_get_free_heap_size(),
         (int) v7_heap_stat(s_sjp.v7, V7_HEAP_STAT_HEAP_SIZE) -
             (int) v7_heap_stat(s_sjp.v7, V7_HEAP_STAT_HEAP_USED));

  fflush(stdout);
  s_sjp.char_processor = process_prompt_char;
}

static void process_here_char(char ch) {
  printf("%c", ch);
  struct mbuf *m = &s_sjp.buf;

  if ((m->len >= 7 && strncmp(&m->buf[m->len - 7], "\r\nEOF\r\n", 7) == 0) ||
      (m->len >= 5 && (strncmp(&m->buf[m->len - 5], "\nEOF\n", 5) == 0 ||
                       strncmp(&m->buf[m->len - 5], "\rEOF\r", 5) == 0))) {
    int end_pos = m->len - (m->buf[m->len - 2] == '\r' ? 7 : 5);
    m->buf[end_pos] = '\0';
    printf("\n");
    process_js(m->buf);
    mbuf_remove(m, m->len);
    mbuf_trim(m);
    show_prompt();
  }
}

static void process_here(int argc, char *argv[], unsigned int param) {
  (void) argc;
  (void) argv;
  (void) param;
  s_sjp.char_processor = process_here_char;
}

static void process_js(const char *cmd) {
  v7_val_t res;
  enum v7_err err;
  struct v7 *v7 = s_sjp.v7;
  s_sjp.running = true;
  {
    struct v7_exec_opts opts;
    memset(&opts, 0, sizeof(opts));
    opts.filename = "repl";
    err = v7_exec_opt(s_sjp.v7, cmd, &opts, &res);
  }
  s_sjp.running = false;

  if (err == V7_SYNTAX_ERROR) {
    printf("Syntax error: %s\n", v7_get_parser_error(v7));
  } else if (err == V7_EXEC_EXCEPTION) {
    miot_print_exception(v7, res, "Exec error");
  } else {
    v7_println(v7, res);
  }

  v7_gc(v7, 1 /* full */);
}

static const char help_str[] =
    "Commands:\n"
    ":help - show this help\n"
    ":here - read all input until a line with EOF in it\n"
    "All other input is treated as JS\n";

static void process_help(int argc, char *argv[], unsigned int param) {
  (void) argc;
  (void) argv;
  (void) param;
  printf(help_str);
}

typedef void (*f_cmdprocessor)(int argc, char *argv[], unsigned int param);

struct firmware_command {
  char *command;
  f_cmdprocessor proc;
  unsigned int param;
};

static void process_prompt_char(char symb);

static const struct firmware_command cmds[] = {
    {"help", &process_help, 0}, {"here", &process_here, 0},
};

static void process_command(struct mbuf *m) {
  const char *cmd = m->buf;
  if (m->len > 0 && *cmd == ':') {
    size_t i;
    for (i = 0; i < ARRAY_SIZE(cmds); i++) {
      if (strncmp(cmd + 1, cmds[i].command, strlen(cmds[i].command)) == 0) {
        cmds[i].proc(0, 0, cmds[i].param);
        break;
      }
    }
    if (i == sizeof(cmds) / sizeof(cmds[0])) {
      printf("Unknown command '%.*s', type :help for the list of commands\n",
             (int) m->len, m->buf);
    }
    if (s_sjp.char_processor == process_prompt_char) show_prompt();
  } else {
    /* skip empty commands */
    if (m->len > 0 && *cmd != '\r' && *cmd != '\n') {
      mbuf_append(m, "", 1); /* NUL-terminate */
      process_js(cmd);
    }
    show_prompt();
  }
  mbuf_remove(m, m->len);
  mbuf_trim(m);
}

static void process_prompt_char(char ch) {
  struct mbuf *m = &s_sjp.buf;

  if (s_sjp.swallow > 0) {
    s_sjp.swallow--;
    m->len--;
    return;
  }

  switch (ch) {
    /*
     * swallow cursor movement escape sequences (and other 3 byte sequences)
     * they are fairly common in random user input and some serial terminals
     * will just move the cursor around if echoed, confusing users.
     */
    case 0x1b:
      s_sjp.swallow = 2;
      break;
    case 0x7f:
    case 0x08:
      m->len--; /* Swallow BS itself. */
      if (m->len > 0) {
        m->len--;
        /* \b only moves the cursor left, let's also clear the char */
        printf("\b \b");
      }
      m->buf[m->len] = '\0';
      break;
    case '\r':
    case '\n':
    case EOF_CHAR:
#if !MIOT_PROMPT_DISABLE_ECHO
      printf("\n");
#endif
      m->len--;
      m->buf[m->len] = '\0';
      process_command(&s_sjp.buf);
      break;
    default:
#if !MIOT_PROMPT_DISABLE_ECHO
      printf("%c", ch); /* echo */
#endif
      break;
  }
}

void miot_prompt_init(struct v7 *v7, int uart_no) {
  memset(&s_sjp, 0, sizeof(s_sjp));

  /* Install prompt if enabled in the config and user's app has not installed
   * a custom UART handler. */
  if (uart_no < 0 || !get_cfg()->debug.enable_prompt ||
      miot_uart_get_dispatcher(uart_no) != NULL) {
    return;
  }

  s_sjp.v7 = v7;
  s_sjp.uart_no = uart_no;

  printf("\n");
  miot_prompt_init_hal();
  show_prompt();
  miot_uart_set_dispatcher(uart_no, miot_prompt_dispatcher, NULL);
  miot_uart_set_rx_enabled(uart_no, true);
}

void miot_prompt_process_char(char ch) {
  struct mbuf *m = &s_sjp.buf;
  if (ch == SIGINT_CHAR) {
    if (s_sjp.running) {
      v7_interrupt(s_sjp.v7);
    } else {
      show_prompt();
    }
  } else if (ch == EOF_CHAR) {
#if MIOT_ENABLE_RPC && MIOT_ENABLE_RPC_CHANNEL_UART
    const int uart_no = s_sjp.uart_no;
    if (uart_no >= 0 && miot_rpc_get_global() != NULL) {
      /* Switch into RPC mode. This will detach our dispatcher. */
      struct mg_rpc_channel *ch =
          mg_rpc_channel_uart(uart_no, false /* wait_for_start_frame */);
      if (ch != NULL) {
        mg_rpc_add_channel(miot_rpc_get_global(), mg_mk_str(""), ch,
                           true /* is_trusted */, false /* send_hello */);
        ch->ch_connect(ch);
      }
      return;
    }
#endif
    /* Else fall through and process as end of command. */
  }
  mbuf_append(m, &ch, 1);
  s_sjp.char_processor(ch);
}

void miot_prompt_dispatcher(struct miot_uart_state *us) {
  uint8_t *cp;
  cs_rbuf_t *rxb = &us->rx_buf;
  while (cs_rbuf_get(rxb, 1, &cp) == 1) {
    cs_rbuf_consume(rxb, 1);
    s_sjp.uart_no = us->uart_no;
    miot_prompt_process_char((char) *cp);

    if (us->dispatcher_cb != miot_prompt_dispatcher) {
      /*
       * Processing of the last char caused char dispatcher to be changed,
       * so we have to exit immediately and not consume the rest of the data
       */
      break;
    }
  }
  fflush(stdout);
}

#endif /* MIOT_ENABLE_JS */
