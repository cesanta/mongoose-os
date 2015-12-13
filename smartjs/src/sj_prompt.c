/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */
#include "sj_prompt.h"
#include "sj_v7_ext.h"
#include "sj_hal.h"
#include "sj_v7_ext.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "v7.h"

#define RX_BUFSIZE 512
#define SIGINT_CHAR 0x03

typedef void (*char_processor_t)(char ch);

struct sj_prompt_state {
  struct v7 *v7;
  char_processor_t char_processor;
  /* TODO(mkm): mbuf? */
  char buf[RX_BUFSIZE];
  int pos;
  int swallow;
};

static struct sj_prompt_state s_sjp;

static void process_js(char *cmd);
static void process_prompt_char(char ch);

static void show_prompt(void) {
  /*
   * Flashnchips relies on prompt ending with "$ " to detect when it's okay
   * to send the next line during file upload.
   */

  /* TODO RTOS(alashkin): RTOS printf doesn't support %lu */
  printf("smartjs %u/%d$ ", (unsigned int) sj_get_free_heap_size(),
         (int) v7_heap_stat(s_sjp.v7, V7_HEAP_STAT_HEAP_SIZE) -
             (int) v7_heap_stat(s_sjp.v7, V7_HEAP_STAT_HEAP_USED));

  fflush(stdout);
  s_sjp.pos = 0;
  s_sjp.char_processor = process_prompt_char;
}

static void process_here_char(char ch) {
  printf("%c", ch);

  if ((s_sjp.pos >= 7 &&
       strncmp(&s_sjp.buf[s_sjp.pos - 7], "\r\nEOF\r\n", 7) == 0) ||
      (s_sjp.pos >= 5 &&
       (strncmp(&s_sjp.buf[s_sjp.pos - 5], "\nEOF\n", 5) == 0 ||
        strncmp(&s_sjp.buf[s_sjp.pos - 5], "\rEOF\r", 5) == 0))) {
    int end_pos = s_sjp.pos - (s_sjp.buf[s_sjp.pos - 2] == '\r' ? 7 : 5);
    s_sjp.buf[end_pos] = '\0';
    printf("\n");
    process_js(s_sjp.buf);
    show_prompt();
  } else {
    if (s_sjp.pos >= RX_BUFSIZE) {
      printf("Input too long\n");
      show_prompt();
    }
  }
}

static void process_here(int argc, char *argv[], unsigned int param) {
  printf("Terminate input with: EOF\n");
  s_sjp.pos = 0;
  s_sjp.char_processor = process_here_char;
}

static void interrupt_char_processor(char ch) {
  if (ch == SIGINT_CHAR) v7_interrupt(s_sjp.v7);
}

static void process_js(char *cmd) {
  s_sjp.char_processor = interrupt_char_processor;
  v7_val_t res;
  enum v7_err err = v7_exec(s_sjp.v7, cmd, &res);
  struct v7 *v7 = s_sjp.v7;

  if (err == V7_SYNTAX_ERROR) {
    printf("Syntax error: %s\n", v7_get_parser_error(v7));
  } else if (err == V7_STACK_OVERFLOW) {
    printf("Stack overflow: %s\n", v7_get_parser_error(v7));
  } else if (err == V7_EXEC_EXCEPTION) {
    sj_print_exception(v7, res, "Exec error");
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

static void process_command(char *cmd) {
  if (*cmd == ':') {
    int i;
    for (i = 0; i < sizeof(cmds) / sizeof(cmds[0]); i++) {
      if (strncmp(cmd + 1, cmds[i].command, strlen(cmds[i].command)) == 0) {
        cmds[i].proc(0, 0, cmds[i].param);
        break;
      }
    }
    if (i == sizeof(cmds) / sizeof(cmds[0])) {
      printf("Unknown command, type :help for the list of commands\n");
    }
    if (s_sjp.char_processor == process_prompt_char) show_prompt();
  } else {
    /* skip empty commands */
    if (*cmd) process_js(cmd);
    show_prompt();
  }
}

static void process_prompt_char(char ch) {
  if (s_sjp.swallow > 0) {
    s_sjp.swallow--;
    s_sjp.pos--;
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
      s_sjp.pos--; /* Swallow BS itself. */
      if (s_sjp.pos > 0) {
        s_sjp.pos--;
        /* \b only moves the cursor left, let's also clear the char */
        printf("\b \b");
      }
      s_sjp.buf[s_sjp.pos] = '\0';
      break;
    case '\n':
#ifndef SJ_PROMPT_DISABLE_ECHO
      printf("\n");
#endif
      s_sjp.pos--;
      s_sjp.buf[s_sjp.pos] = '\0';
      process_command(s_sjp.buf);
      s_sjp.pos = 0;
      break;
    case '\r':
      break;
    default:
#ifndef SJ_PROMPT_DISABLE_ECHO
      printf("%c", ch); /* echo */
#endif
      break;
  }
}

void sj_prompt_init(struct v7 *v7) {
  memset(&s_sjp, 0, sizeof(s_sjp));

  /* TODO(alashkin): load cfg from flash */
  s_sjp.v7 = v7;

  printf("\n");
  sj_prompt_init_hal();
  show_prompt();
}

void sj_prompt_process_char(char ch) {
  if (s_sjp.pos >= RX_BUFSIZE - 1) {
    printf("\nCommand buffer overflow.\n");
    s_sjp.pos = 0;
    show_prompt();
  }
  s_sjp.buf[s_sjp.pos++] = ch;
  s_sjp.buf[s_sjp.pos] = '\0';
  s_sjp.char_processor(ch);
}
