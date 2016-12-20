/*
 * Copyright (c) 2014-2016 Cesanta Software Limited
 * All rights reserved
 */

#if MIOT_ENABLE_GPIO_API

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <poll.h>

#include "fw/src/miot_gpio.h"
#include "fw/src/miot_gpio_hal.h"

int gpio_export(int gpio_no) {
  char buf[50];
  struct stat s = {0};
  int fd, res;

  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d", gpio_no);
  if (stat(buf, &s) == 0 && s.st_mode & S_IFDIR) {
    /* GPIO already exported */
    return 0;
  }

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (fd < 0) {
    fprintf(stderr, "Cannot open /sys/class/gpio/export: %d (%s)\n", errno,
            strerror(errno));
    return -1;
  };

  snprintf(buf, sizeof(buf), "%d", gpio_no);
  res = write(fd, buf, strlen(buf));
  if (res < 0) {
    fprintf(stderr, "Cannot export GPIO %d: %d (%s)\n", gpio_no, errno,
            strerror(errno));
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

bool gpio_helper(int gpio_no, const char *file_name_templ, const char *val) {
  int fd, res;
  char buf[50];
  snprintf(buf, sizeof(buf), file_name_templ, gpio_no);

  fd = open(buf, O_WRONLY);
  if (fd < 0) {
    fprintf(stderr, "Cannot open %s: %d (%s)\n", buf, errno, strerror(errno));
    return false;
  };

  res = write(fd, val, strlen(val));

  if (res < 0) {
    fprintf(stderr, "Cannot write to %s value %s: %d (%s)\n", buf, val, errno,
            strerror(errno));
    close(fd);
    return false;
  }

  close(fd);

  return true;
}

/* 0 = IN, 1 = OUT */
bool gpio_set_direction(int gpio_no, enum miot_gpio_mode d) {
  return gpio_helper(gpio_no, "/sys/class/gpio/gpio%d/direction",
                     d == MIOT_GPIO_MODE_INPUT ? "in" : "out");
}

bool gpio_set_value(int gpio_no, bool value) {
  return gpio_helper(gpio_no, "/sys/class/gpio/gpio%d/value",
                     value == 0 ? "0" : "1");
}

bool gpio_get_value(int gpio_no) {
  char val, buf[50];
  int fd, res;

  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", gpio_no);
  fd = open(buf, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Cannot open %s: %d (%s)\n", buf, errno, strerror(errno));
    return false;
  };

  res = read(fd, &val, sizeof(val));
  if (res < 0) {
    fprintf(stderr, "Cannot read from %s: %d (%s)\n", buf, errno,
            strerror(errno));
    close(fd);
    return false;
  }

  close(fd);

  return (val == '1');
}

bool gpio_set_edge(int gpio_no, enum miot_gpio_int_mode edge) {
  static const char *edge_names[] = {"none", "rising", "falling", "both"};
  /* signedness of enum is implementation defined */
  if (((unsigned int) edge) >= (sizeof(edge_names) / sizeof(edge_names[0]))) {
    fprintf(stderr, "Invalid egde value\n");
    return false;
  }

  return gpio_helper(gpio_no, "/sys/class/gpio/gpio%d/edge", edge_names[edge]);
}

#define HANDLER_MAX_COUNT 10
#define POLL_TIMEOUT 100

struct gpio_event {
  int gpio_no;
  int fd;
  struct gpio_event *next;
  int enabled;
};

static struct gpio_event *s_events = NULL;

struct gpio_event *get_gpio_event(int gpio_no) {
  struct gpio_event *ev = s_events;

  while (ev != NULL) {
    if (ev->gpio_no == gpio_no) {
      return ev;
    }
    ev = ev->next;
  }

  return NULL;
}

void miot_gpio_dev_int_done(int pin) {
  (void) pin;
}

void gpio_remove_handler(int gpio_no) {
  struct gpio_event *ev = s_events, *prev_ev = NULL;

  while (ev != NULL) {
    if (ev->gpio_no == gpio_no) {
      close(ev->fd);
      if (prev_ev != NULL) {
        prev_ev->next = ev->next;
      } else {
        s_events = ev->next;
      }
      free(ev);
      return;
    }

    prev_ev = ev;
    ev = ev->next;
  }
}

bool miot_gpio_dev_set_int_mode(int gpio_no, enum miot_gpio_int_mode mode) {
  int fd, res;
  char tmp, buf[50];
  struct gpio_event *new_ev;

  if (mode == MIOT_GPIO_INT_NONE) {
    gpio_remove_handler(gpio_no);
    return true;
  }

  if (get_gpio_event(gpio_no) != NULL) {
    /* doesn't support several handlers for the same gpio */
    fprintf(stderr, "ISR for GPIO%d already installed\n", gpio_no);
    return false;
  }

  if (!gpio_set_edge(gpio_no, mode)) {
    return false;
  }

  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", gpio_no);
  fd = open(buf, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Cannot open %s: %d (%s)\n", buf, errno, strerror(errno));
    return false;
  };

  /*
   * in order to use poll we have to
   * move file pointer behind gpio value
   */
  res = read(fd, &tmp, sizeof(tmp));
  if (res < 0) {
    fprintf(stderr, "Cannot read from %s: %d (%s)\n", buf, errno,
            strerror(errno));
    return false;
  };

  new_ev = calloc(1, sizeof(*new_ev));
  new_ev->gpio_no = gpio_no;
  new_ev->next = s_events;
  new_ev->fd = fd;
  new_ev->enabled = 1;
  s_events = new_ev;

  return true;
}

bool miot_gpio_enable_int(int pin) {
  /* FIXME */
  (void) pin;
  return false;
}

bool miot_gpio_disable_int(int pin) {
  /* FIXME */
  (void) pin;
  return false;
}

int gpio_poll(void) {
  struct pollfd fdset[HANDLER_MAX_COUNT];
  struct gpio_event *events_tmp[HANDLER_MAX_COUNT];

  struct gpio_event *ev = s_events;
  int fd_count = 0, res, i;
  char val;

  if (s_events == NULL) {
    /* No handlers, ok */
    return 0;
  }

  memset(&fdset, 0, sizeof(fdset));
  while (ev != NULL) {
    if (ev->enabled) {
      fdset[fd_count].fd = ev->fd;
      fdset[fd_count].events = POLLPRI;
      events_tmp[fd_count] = ev;
      fd_count++;
    }
    ev = ev->next;
  }

  res = poll(fdset, fd_count, POLL_TIMEOUT);

  if (res < 0) {
    fprintf(stderr, "Failed to poll %d (%s)\n", errno, strerror(errno));
    return -1;
  }

  if (res == 0) {
    /* Timeout - ok */
    return 1;
  }

  for (i = 0; i < fd_count; i++) {
    if ((fdset[i].revents & POLLIN) != 0) {
      lseek(fdset[i].fd, 0, SEEK_SET);
      res = read(fdset[i].fd, &val, sizeof(val));
      events_tmp[i]->enabled = 0;
      miot_gpio_dev_int_cb(events_tmp[i]->gpio_no);
    }
  }

  return 1;
}

/* HAL functions */
bool miot_gpio_set_mode(int pin, enum miot_gpio_mode mode) {
  if (gpio_export(pin) < 0) {
    return false;
  }
  return gpio_set_direction(pin, mode);
}

bool miot_gpio_set_pull(int pin, enum miot_gpio_pull_type pull) {
  if (pull == MIOT_GPIO_PULL_NONE) return true;
  /*
   * Documented API for using internal pullup/pulldown
   * resistors requires kernel built with special flags
   * Basically, it works in Raspberian only
   * (and causes crash in another Linux)
   * Do not support internal pulling for now
   */
  fprintf(stderr, "Pullup/pulldown aren't supported\n");
  (void) pin;
  return false;
}

void miot_gpio_write(int pin, bool level) {
  gpio_set_value(pin, level);
}

bool miot_gpio_read(int pin) {
  return gpio_get_value(pin);
}


enum miot_init_result miot_gpio_dev_init(void) {
  return MIOT_INIT_OK;
}

#endif /* MIOT_ENABLE_JS && MIOT_ENABLE_GPIO_API */
