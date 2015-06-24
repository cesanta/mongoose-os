/*
 * Copyright (c) 2015 Cesanta Software Limited
 * All rights reserved
 */
#include <stdarg.h>
#include <stdlib.h>

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "v7.h"
#include "mem.h"

#include "v7_uart.h"

#define RX_BUFSIZE 0x100

extern struct v7 *v7;

ICACHE_FLASH_ATTR void process_js(char *cmd);

ICACHE_FLASH_ATTR static void show_prompt(void) {
  printf("smartjs %u$ ", system_get_free_heap_size());
}

static uart_process_char_t here_old;

int here_pos = 0;
ICACHE_FLASH_ATTR static void process_here_char(char ch) {
  /* TODO(mkm): mbuf? */
  static char here_buf[512];
  here_buf[here_pos] = ch;
  if (here_pos > 0) printf("%c", ch);

  if (here_pos > 3 && strncmp(&here_buf[here_pos - 6], "\r\nEOF\r\n", 7) == 0) {
    here_buf[here_pos - 4] = '\0';
    process_js(&here_buf[1]);
    uart_process_char = here_old;
    show_prompt();
  } else {
    here_pos++;
  }
}

ICACHE_FLASH_ATTR static void process_here(int argc, char *argv[],
                                           unsigned int param) {
  printf("Terminate input with: EOF\n");
  here_pos = 0;
  here_old = uart_process_char;
  uart_process_char = process_here_char;
}

ICACHE_FLASH_ATTR static void interrupt_cb(char ch) {
  (void) ch;
  v7_interrupt(v7);
}

ICACHE_FLASH_ATTR void process_js(char *cmd) {
  uart_process_char_t old_int = uart_interrupt_cb;
  uart_interrupt_cb = interrupt_cb;
  static char result_str[10];
  v7_val_t v;
  int res = v7_exec(v7, &v, cmd);

  if (res == V7_SYNTAX_ERROR) {
    printf("Syntax error: %s\n", v7_get_parser_error(v7));
  } else if (res == V7_STACK_OVERFLOW) {
    printf("Stack overflow: %s\n", v7_get_parser_error(v7));
  } else {
    char *p;
    p = v7_to_json(v7, v, result_str, sizeof(result_str));

    if (res == V7_EXEC_EXCEPTION) {
      printf("Exec error:");
    }

    printf("%s\n", p);

    if (p != result_str) {
      free(p);
    }
  }

  v7_gc(v7);
  uart_interrupt_cb = old_int;
}

ICACHE_FLASH_ATTR static void process_help(int argc, char *argv[],
                                           unsigned int param) {
  char *help_str =
      "Commands:\n"
      ":help - show this help\n"
      ":here - read all input until a line with EOF in it\n"
      "All other input is treated as JS\n";

  printf(help_str);
}

typedef void (*f_cmdprocessor)(int argc, char *argv[], unsigned int param);

struct firmware_command {
  char *command;
  f_cmdprocessor proc;
  unsigned int param;
};

void process_prompt_char(char symb);

static const struct firmware_command cmds[] = {{"help", &process_help, 0},
                                               {"here", &process_here, 0}};

ICACHE_FLASH_ATTR void process_command(char *cmd) {
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
    if (uart_process_char == process_prompt_char) show_prompt();
  } else {
    /* skip empty commands */
    if (*cmd) process_js(cmd);
    show_prompt();
  }
}

ICACHE_FLASH_ATTR void process_prompt_char(char symb) {
  static char recv_buf[RX_BUFSIZE] = {0};
  static int recv_buf_pos = 0;
  static int swallow = 0;

  if (swallow > 0) {
    swallow--;
    return;
  }

  switch (symb) {
    /*
     * swallow cursor movement escape sequences (and other 3 byte sequences)
     * they are fairly common in random user input and some serial terminals
     * will just move the cursor around if echoed, confusing users.
     */
    case 0x1b:
      swallow = 2;
      break;
    case 0x7f:
    case 0x08:
      if (recv_buf_pos > 0) {
        recv_buf_pos--;
        recv_buf[recv_buf_pos] = '\0';
        /* \b only moves the cursor left, let's also clear the char */
        printf("\b \b");
      }
      break;
    case '\r':
      printf("\n");
      process_command(recv_buf);
      recv_buf_pos = 0;
      memset(recv_buf, 0, sizeof(recv_buf));
      break;
    case '\n':
      break;
    default:
      printf("%c", symb); /* echo */
      recv_buf[recv_buf_pos++] = symb;
      break;
  }

  if (recv_buf_pos > sizeof(recv_buf)) {
    /*
     * TODO(alashkin): make handle of long command
     * more smarter (write to flash?)
     */
    printf("Command is too long\n");
    recv_buf_pos = 0;
    memset(recv_buf, 0, sizeof(recv_buf));
  }
}

ICACHE_FLASH_ATTR int v7_serial_prompt_init(int baud_rate) {
  /* TODO(alashkin): load cfg from flash */

  uart_process_char = process_prompt_char;
  printf("\n");
  show_prompt();
}
