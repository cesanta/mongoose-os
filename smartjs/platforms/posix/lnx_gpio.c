#ifndef SJ_DISABLE_GPIO

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
#include <sj_gpio.h>

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

int gpio_helper(int gpio_no, const char *file_name_templ, const char *val) {
  int fd, res;
  char buf[50];
  snprintf(buf, sizeof(buf), file_name_templ, gpio_no);

  fd = open(buf, O_WRONLY);
  if (fd < 0) {
    fprintf(stderr, "Cannot open %s: %d (%s)\n", buf, errno, strerror(errno));
    return -1;
  };

  res = write(fd, val, strlen(val));

  if (res < 0) {
    fprintf(stderr, "Cannot write to %s value %s: %d (%s)\n", buf, val, errno,
            strerror(errno));
    close(fd);
    return -1;
  }

  close(fd);

  return 0;
}

/* 0 = IN, 1 = OUT */
int gpio_set_direction(int gpio_no, enum gpio_mode d) {
  return gpio_helper(gpio_no, "/sys/class/gpio/gpio%d/direction",
                     d == GPIO_MODE_INPUT ? "in" : "out");
}

int gpio_set_value(int gpio_no, enum gpio_level val) {
  return gpio_helper(gpio_no, "/sys/class/gpio/gpio%d/value",
                     val == GPIO_LEVEL_LOW ? "0" : "1");
}

int gpio_get_value(int gpio_no) {
  char val, buf[50];
  int fd, res;

  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", gpio_no);
  fd = open(buf, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Cannot open %s: %d (%s)\n", buf, errno, strerror(errno));
    return -1;
  };

  res = read(fd, &val, sizeof(val));
  if (res < 0) {
    fprintf(stderr, "Cannot read from %s: %d (%s)\n", buf, errno,
            strerror(errno));
    close(fd);
    return -1;
  }

  close(fd);

  return val == '0' ? 0 : 1;
}

int gpio_set_edge(int gpio_no, enum gpio_int_mode edge) {
  static const char *edge_names[] = {"none", "rising", "falling", "both"};
  if (edge < 0 || edge >= (sizeof(edge_names) / sizeof(edge_names[0]))) {
    fprintf(stderr, "Invalid egde value\n");
    return -1;
  }

  return gpio_helper(gpio_no, "/sys/class/gpio/gpio%d/edge", edge_names[edge]);
}

#define HANDLER_MAX_COUNT 10
#define POLL_TIMEOUT 100

typedef void (*gpio_callback)(int pin, int level);

struct gpio_event {
  int gpio_no;
  gpio_callback callback;
  int fd;
  struct gpio_event *next;
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

int gpio_set_handler(int gpio_no, gpio_callback callback) {
  int fd, res;
  char tmp, buf[50];
  struct gpio_event *new_ev;

  if (get_gpio_event(gpio_no) != NULL) {
    /* doesn't support several handlers for the same gpio */
    fprintf(stderr, "ISR for GPIO%d already installed\n", gpio_no);
    return -1;
  }

  snprintf(buf, sizeof(buf), "/sys/class/gpio/gpio%d/value", gpio_no);
  fd = open(buf, O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "Cannot open %s: %d (%s)\n", buf, errno, strerror(errno));
    return -1;
  };

  /*
   * in order to use poll we have to
   * move file pointer behind gpio value
   */
  res = read(fd, &tmp, sizeof(tmp));
  if (res < 0) {
    fprintf(stderr, "Cannot read from %s: %d (%s)\n", buf, errno,
            strerror(errno));
    return -1;
  };

  new_ev = calloc(1, sizeof(*new_ev));
  new_ev->gpio_no = gpio_no;
  new_ev->callback = callback;
  new_ev->next = s_events;
  new_ev->fd = fd;
  s_events = new_ev;

  return 0;
}

int gpio_poll() {
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
    fdset[fd_count].fd = ev->fd;
    fdset[fd_count].events = POLLPRI;
    events_tmp[fd_count] = ev;
    fd_count++;

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
      events_tmp[i]->callback(events_tmp[i]->gpio_no, val == '0' ? 0 : 1);
    }
  }

  return 1;
}

/* HAL functions */
int sj_gpio_set_mode(int pin, enum gpio_mode mode, enum gpio_pull_type pull) {
  if (mode == GPIO_MODE_INOUT) {
    fprintf(stderr, "Inout mode is not supported\n");
    return -1;
  }

  if (pull != GPIO_PULL_FLOAT) {
    /*
     * Documented API for using internal pullup/pulldown
     * resistors requires kernel built with special flags
     * Basically, it works in Raspberian only
     * (and causes crash in another Linux)
     * Do not support internal pulling for now
     */
    fprintf(stderr, "Pullup/pulldown aren't supported\n");
    return -1;
  }

  if (gpio_export(pin) < 0) {
    return -1;
  }

  if (mode == GPIO_MODE_INT) {
    /*
     * GPIO should be in "IN" mode
     * for using /sys/edge
     */
    mode = GPIO_MODE_INPUT;
  }

  return gpio_set_direction(pin, mode);
}

int sj_gpio_write(int pin, enum gpio_level level) {
  return gpio_set_value(pin, level);
}

enum gpio_level sj_gpio_read(int pin) {
  return gpio_get_value(pin);
}

static f_gpio_intr_handler_t proxy_handler;

void sj_gpio_intr_init(f_gpio_intr_handler_t cb) {
  proxy_handler = cb;
}

int sj_gpio_intr_set(int pin, enum gpio_int_mode type) {
  if (type == GPIO_INTR_DISABLE) {
    gpio_remove_handler(pin);
    return 0;
  }

  if (gpio_set_edge(pin, type) < 0) {
    return -1;
  }

  return gpio_set_handler(pin, proxy_handler);
}

#else

int gpio_poll() {
  return 0;
}

#endif
