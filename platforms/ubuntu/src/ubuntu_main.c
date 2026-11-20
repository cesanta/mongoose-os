/*
 * Copyright 2019 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include "rpa_queue.h"

#include "mgos_debug_internal.h"
#include "mgos_init_internal.h"
#include "mgos_mongoose.h"
#include "mgos_mongoose_internal.h"
#include "mgos_net_hal.h"
#include "mgos_sys_config.h"
#include "mgos_uart_internal.h"
#include "mgos_utils.h"
#include "ubuntu.h"

extern const char *build_version, *build_id;
extern const char *mg_build_version, *mg_build_id;

struct ubuntu_flags Flags;

static bool mongoose_running = false;
static pid_t s_parent, s_child;

struct cb_info {
  void (*cb)(void *arg);
  void *cb_arg;
};

static rpa_queue_t *s_cbs_main = NULL;
static rpa_queue_t *s_cbs_bg = NULL;

struct mgos_rlock_type *s_mgos_lock = NULL;

static void ubuntu_sigint_handler(int sig UNUSED_ARG) {
  mongoose_running = false;
}

static void *ubuntu_bg_task(void *arg UNUSED_ARG) {
  LOG(LL_DEBUG, ("Background task started"));
  struct cb_info *cbi = NULL;
  while (rpa_queue_pop(s_cbs_bg, (void **) &cbi)) {
    cbi->cb(cbi->cb_arg);
    free(cbi);
  }
  LOG(LL_DEBUG, ("Background task exiting"));
  return NULL;
}

static int ubuntu_mongoose(void) {
  enum mgos_init_result r;
  pthread_t bg_task;

  s_mgos_lock = mgos_rlock_create();

  assert(rpa_queue_create(&s_cbs_main, 32));
  assert(rpa_queue_create(&s_cbs_bg, 32));
  assert(pthread_create(&bg_task, NULL /* attr */, ubuntu_bg_task, NULL) == 0);

  ubuntu_set_boottime();
  ubuntu_set_nsleep100();
  if (!ubuntu_cap_init()) {
    return -2;
  }

  r = mongoose_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR,
        ("mongoose_init=%d (expecting %d), exiting", r, MGOS_INIT_OK));
    mgos_system_restart();
    rpa_queue_term(s_cbs_bg);
    pthread_join(bg_task, NULL /* retval */);
    return -3;
  }
  mongoose_running = true;
  struct sigaction sa = {
      .sa_handler = ubuntu_sigint_handler,
  };
  sigaction(SIGINT, &sa, NULL);
  while (mongoose_running) {
    struct cb_info *cbi = NULL;
    while (rpa_queue_trypop(s_cbs_main, (void **) &cbi)) {
      cbi->cb(cbi->cb_arg);
      free(cbi);
    }
    mongoose_poll(1);
  }
  rpa_queue_term(s_cbs_bg);
  pthread_join(bg_task, NULL /* retval */);
  return 0;
}

bool mgos_invoke_cb(mgos_cb_t cb, void *arg, uint32_t flags) {
  struct cb_info *cbi = (struct cb_info *) calloc(1, sizeof(*cbi));
  if (cbi == NULL) return false;
  cbi->cb = cb;
  cbi->cb_arg = arg;
  return rpa_queue_trypush(
      ((flags & MGOS_INVOKE_CB_F_BG_TASK) ? s_cbs_bg : s_cbs_main), cbi);
}

static int ubuntu_main(void) {
  for (;;) {
    int wstatus;
    pid_t wpid;

    int res = ubuntu_ipc_handle(1000);
    if (res == UBUNTU_IPC_RES_RESTART) {
      waitpid(-1, &wstatus, 0);
      return UBUNTU_IPC_RES_RESTART;
    }
    if (res < 0) {
      LOGM(LL_ERROR, ("IPC error"));
      kill(s_child, SIGTERM);
      return res;
    }
    if (!ubuntu_wdt_ok()) {
      LOGM(LL_ERROR, ("Watchdog timeout"));
      kill(s_child, SIGTERM);
      return -5;
    }

    wpid = waitpid(-1, &wstatus, WNOHANG);
    if (wpid > 0) {
      return WEXITSTATUS(wstatus);
    }
  }
}

int main(int argc, char *argv[]) {
  int ret = -1;

  if (!ubuntu_flags_init(argc, argv)) {
    return -1;
  }

  if (!ubuntu_ipc_init()) {
    LOGM(LL_ERROR, ("Opening stream socket pair failed"));
    return -1;
  }
  s_parent = getpid();
  if ((s_child = fork()) == -1) {
    LOGM(LL_ERROR, ("Forking child failed"));
    return -2;
  } else if (s_child) {
    // Parent
    LOGM(LL_INFO,
         ("PIDs: parent=%d child=%d uid=%d gid=%d euid=%d egid=%d", s_parent,
          s_child, getuid(), getgid(), geteuid(), getegid()));
    ubuntu_ipc_init_main();
    ret = ubuntu_main();
    ubuntu_ipc_destroy_main();
  } else {
    // Child
    ubuntu_ipc_init_mongoose();
    ubuntu_mongoose();
    ubuntu_ipc_destroy_mongoose();
    return 0;
  }
  if (ret == UBUNTU_IPC_RES_RESTART) {
    // Need to NULL-terminate argv for exec.
    char *new_argv[1000] = {0};
    for (int i = 0; i < argc; i++) {
      new_argv[i] = argv[i];
    }
    LOGM(LL_INFO, ("Restarting"));
    execv(new_argv[0], new_argv);
  }
  LOGM((ret == 0 ? LL_INFO : LL_ERROR), ("Exiting %d. Have a great day!", ret));
  return ret;
}

static void dummy_handler(struct mg_connection *nc, int ev, void *ev_data,
                          void *user_data) {
  (void) nc;
  (void) ev;
  (void) ev_data;
  (void) user_data;
}

void mongoose_schedule_poll(bool from_isr) {
  mg_broadcast(mgos_get_mgr(), dummy_handler, NULL, 0);
  (void) from_isr;
}

static void ubuntu_net_up(void *arg) {
  struct mgos_net_ip_info ipaddr;
  char ip[INET_ADDRSTRLEN], netmask[INET_ADDRSTRLEN], gateway[INET_ADDRSTRLEN];
  mgos_eth_dev_get_ip_info(0, &ipaddr);
  inet_ntop(AF_INET, (void *) &ipaddr.gw.sin_addr, gateway, INET_ADDRSTRLEN);
  inet_ntop(AF_INET, (void *) &ipaddr.ip.sin_addr, ip, INET_ADDRSTRLEN);
  inet_ntop(AF_INET, (void *) &ipaddr.netmask.sin_addr, netmask,
            INET_ADDRSTRLEN);
  LOG(LL_INFO, ("Network: ip=%s netmask=%s gateway=%s", ip, netmask, gateway));
  mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, 0, MGOS_NET_EV_CONNECTING);
  mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, 0, MGOS_NET_EV_CONNECTED);
  mgos_net_dev_event_cb(MGOS_NET_IF_TYPE_ETHERNET, 0, MGOS_NET_EV_IP_ACQUIRED);
  (void) arg;
}

void mgos_lock(void) {
  mgos_rlock(s_mgos_lock);
}

void mgos_unlock(void) {
  mgos_runlock(s_mgos_lock);
}

enum mgos_init_result mongoose_init(void) {
  enum mgos_init_result r;
  int cpu_freq;
  size_t heap_size, free_heap_size;

  r = mgos_uart_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("Failed to mgos_uart_init: %d", r));
    return r;
  }

  r = mgos_debug_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("Failed to mgos_debug_init: %d", r));
    return r;
  }

  r = mgos_debug_uart_init();
  if (r != MGOS_INIT_OK) {
    LOG(LL_ERROR, ("Failed to mgos_debug_uart_init: %d", r));
    return r;
  }

  cs_log_set_level(MGOS_EARLY_DEBUG_LEVEL);

  setvbuf(stdout, NULL, _IOLBF, 256);
  setvbuf(stderr, NULL, _IOLBF, 256);

  LOG(LL_INFO, ("Mongoose OS %s (%s)", mg_build_version, mg_build_id));
  LOG(LL_INFO, ("%s %s (%s)", MGOS_APP, build_version, build_id));

  cpu_freq = (int) (mgos_get_cpu_freq() / 1000000);
  heap_size = mgos_get_heap_size();
  free_heap_size = mgos_get_free_heap_size();
  LOG(LL_INFO, ("CPU: %d MHz, heap: %lu total, %lu free", cpu_freq,
                (unsigned long) heap_size, (unsigned long) free_heap_size));

  mgos_invoke_cb(ubuntu_net_up, NULL, 0);

  srand(time(NULL) + getpid());

  return mgos_init();
}
