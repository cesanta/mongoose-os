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

#include <fnmatch.h>

#include "mgos_system.h"
#include "ubuntu.h"
#include "ubuntu_ipc.h"

struct ubuntu_pipe s_pipe;

static int ubuntu_ipc_handle_open(const char *pathname, int flags) {
  const char *patterns[] = {"/dev/i2c-*", "/dev/spidev*.*", "/proc/cpuinfo",
                            "/sys/class/net/*/address", "/proc/net/route",
                            NULL};
  int i;
  bool ok = false;

  for (i = 0; patterns[i]; i++) {
    if (0 == fnmatch(patterns[i], pathname, FNM_PATHNAME)) {
      ok = true;
      break;
    }
  }
  if (!ok) {
    LOG(LL_ERROR, ("Refusing to open '%s'", pathname));
    return -1;
  }
  return open(pathname, flags);
}

bool ubuntu_ipc_handle(uint16_t timeout_ms) {
  fd_set rfds;
  struct timeval tv;
  int retval;
  size_t _len;
  struct msghdr msg;
  struct iovec iov[1];
  struct ubuntu_pipe_message iovec_payload;
  union {
    struct cmsghdr cm;
    char control[CMSG_SPACE(sizeof(int))];
  } control_un;
  int fd = -1;

  FD_ZERO(&rfds);
  FD_SET(s_pipe.main_fd, &rfds);

  tv.tv_sec = 0;
  tv.tv_usec = timeout_ms * 1000;

  //  LOG(LL_INFO, ("Selecting for %u ms", timeout_ms));
  retval = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
  if (retval < 0) {
    LOGM(LL_ERROR, ("Cannot not select"));
    return false;
  } else if (retval == 0) {
    //    LOG(LL_INFO, ("No data within %u ms", timeout_ms));
    return true;
  }

  memset(&msg, 0, sizeof(struct msghdr));
  memset(&iovec_payload, 0, sizeof(struct ubuntu_pipe_message));
  iov[0].iov_base = (void *) &iovec_payload;
  iov[0].iov_len = sizeof(struct ubuntu_pipe_message);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;

  _len = recvmsg(s_pipe.main_fd, &msg, 0);
  if (_len <= 0) {
    return false;
  }
  // LOG(LL_INFO, ("Received: cmd=%d len=%u msg='%.*s'", iovec_payload.cmd,
  // iovec_payload.len, (int)iovec_payload.len, (char *)iovec_payload.data));

  iovec_payload.len = 0;
  // Handle command
  switch (iovec_payload.cmd) {
    case UBUNTU_CMD_WDT:
      ubuntu_wdt_feed();
      break;

    case UBUNTU_CMD_WDT_EN:
      ubuntu_wdt_enable();
      break;

    case UBUNTU_CMD_WDT_DIS:
      ubuntu_wdt_disable();
      break;

    case UBUNTU_CMD_WDT_TIMEOUT: {
      int secs;
      memcpy(&secs, &iovec_payload.data, sizeof(int));
      ubuntu_wdt_set_timeout(secs);
      break;
    }

    case UBUNTU_CMD_OPEN: {
      const char *fn;
      int flags;
      struct cmsghdr *cmptr;

      fn = (const char *) iovec_payload.data;

      memcpy(&flags, &iovec_payload.data[strlen(fn) + 1], sizeof(int));
      fd = ubuntu_ipc_handle_open(fn, flags);
      if (fd > 0) {
        // Add control message here, see Stevens Unix Network Programming
        // page 428 functions Write_fd() and Read_fd()
        // LOG(LL_INFO, ("Opened '%s' as fd=%d", fn, fd));
        msg.msg_control = control_un.control;
        msg.msg_controllen = sizeof(control_un.control);

        cmptr = CMSG_FIRSTHDR(&msg);
        cmptr->cmsg_len = CMSG_LEN(sizeof(int));
        cmptr->cmsg_level = SOL_SOCKET;
        cmptr->cmsg_type = SCM_RIGHTS;

        *((int *) CMSG_DATA(cmptr)) = fd;
      }
      break;
    }

    case UBUNTU_CMD_PING:
    default:
      iovec_payload.len = 5;
      memcpy(&iovec_payload.data, "PONG!", iovec_payload.len);
  }

  iov[0].iov_base = (void *) &iovec_payload;
  iov[0].iov_len = iovec_payload.len + 2;
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  _len = sendmsg(s_pipe.main_fd, &msg, 0);
  if (_len <= 0) {
    return false;
  }
  if (fd > 0) {
    close(fd);  // Close the UBUNTU_CMD_OPEN fd in parent
  }
  //  LOG(LL_INFO, ("Sent: cmd=%d len=%u msg='%.*s' fd=%d", iovec_payload.cmd,
  //  iovec_payload.len, (int)iovec_payload.len, (char *)iovec_payload.data,
  //  fd));
  return true;
}

bool ubuntu_ipc_init(void) {
  int fd[2];

  if (s_pipe.lock) {
    mgos_rlock_destroy(s_pipe.lock);
  }
  s_pipe.lock = mgos_rlock_create();
  if (0 != socketpair(AF_UNIX, SOCK_STREAM, 0, fd)) {
    LOG(LL_ERROR, ("Can't create socketpair(): %s", strerror(errno)));
    return false;
  }
  s_pipe.main_fd = fd[1];
  s_pipe.mongoose_fd = fd[0];

  return true;
}

bool ubuntu_ipc_init_main(void) {
  if (!s_pipe.lock) {
    return false;
  }
  close(s_pipe.mongoose_fd);
  return true;
}

bool ubuntu_ipc_init_mongoose(void) {
  if (!s_pipe.lock) {
    return false;
  }
  close(s_pipe.main_fd);
  return true;
}

bool ubuntu_ipc_destroy_main(void) {
  if (!s_pipe.lock) {
    return false;
  }
  mgos_rlock_destroy(s_pipe.lock);
  close(s_pipe.main_fd);
  close(s_pipe.mongoose_fd);
  return true;
}

bool ubuntu_ipc_destroy_mongoose(void) {
  if (!s_pipe.lock) {
    return false;
  }
  mgos_rlock_destroy(s_pipe.lock);
  close(s_pipe.main_fd);
  close(s_pipe.mongoose_fd);
  return true;
}
