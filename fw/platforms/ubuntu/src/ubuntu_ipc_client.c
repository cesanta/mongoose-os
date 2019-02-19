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

extern struct ubuntu_pipe s_pipe;

static bool ubuntu_ipc_cmd(const struct ubuntu_pipe_message *in,
                           struct ubuntu_pipe_message *out) {
  ssize_t len;
  bool ret = false;
  struct msghdr msg;
  struct iovec iov[1];

  if (!in || !out) {
    return false;
  }

  memset(&msg, 0, sizeof(struct msghdr));
  iov[0].iov_base = (void *) in;
  iov[0].iov_len = in->len + 2;
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;

  mgos_rlock(s_pipe.lock);
  len = sendmsg(s_pipe.mongoose_fd, &msg, 0);
  if (len < 2) {
    LOG(LL_ERROR, ("Cannot write message %d %d", (int) len, errno));
    goto exit;
  }

  iov[0].iov_base = (void *) out;
  iov[0].iov_len = sizeof(struct ubuntu_pipe_message);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  len = recvmsg(s_pipe.mongoose_fd, &msg, 0);
  if (len < 2) {
    LOG(LL_ERROR, ("Cannot read message %d %d", (int) len, errno));
    goto exit;
  }

exit:
  mgos_runlock(s_pipe.lock);
  return ret;
}

int ubuntu_ipc_open(const char *pathname, int flags) {
  ssize_t len;
  struct msghdr msg;
  struct iovec iov[1];
  struct ubuntu_pipe_message iovec_payload;

  static union {
    struct cmsghdr cm;
    char control[CMSG_SPACE(sizeof(int))];
  } control_un;
  struct cmsghdr *cmptr;

  int fd = -1;

  if (!pathname) {
    goto exit;
  }
  if (strlen(pathname) > 250) {
    goto exit;
  }

  memset(&iovec_payload, 0, sizeof(struct ubuntu_pipe_message));
  iovec_payload.cmd = UBUNTU_CMD_OPEN;
  iovec_payload.len = strlen(pathname) + 1 + sizeof(int);
  memcpy(&iovec_payload.data, pathname, strlen(pathname));
  memcpy(&iovec_payload.data[strlen(pathname) + 1], &flags, sizeof(int));
  memset(&msg, 0, sizeof(struct msghdr));
  iov[0].iov_base = (void *) &iovec_payload;
  iov[0].iov_len = iovec_payload.len + 2;
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;

  mgos_rlock(s_pipe.lock);
  len = sendmsg(s_pipe.mongoose_fd, &msg, 0);
  if (len < 2) {
    LOG(LL_ERROR, ("Cannot write message %d", (int) len));
    goto exit;
  }

  memset(&iovec_payload, 0, sizeof(struct ubuntu_pipe_message));
  iov[0].iov_base = (void *) &iovec_payload;
  iov[0].iov_len = sizeof(struct ubuntu_pipe_message);
  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);

  len = recvmsg(s_pipe.mongoose_fd, &msg, 0);
  if (len < 2) {
    LOG(LL_ERROR, ("Cannot read message"));
    goto exit;
  }

  if ((cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
      cmptr->cmsg_len == CMSG_LEN(sizeof(int)) &&
      cmptr->cmsg_level == SOL_SOCKET && cmptr->cmsg_type == SCM_RIGHTS) {
    fd = *((int *) CMSG_DATA(cmptr));
  }

// LOG(LL_INFO, ("Received: cmd=%u len=%u msg='%.*s', fd=%d", iovec_payload.cmd,
// iovec_payload.len, (int)iovec_payload.len, (char *)iovec_payload.data, fd));
exit:
  mgos_runlock(s_pipe.lock);
  return fd;

  (void) flags;
}

void mgos_wdt_set_timeout(int secs) {
  struct ubuntu_pipe_message out, in;

  mgos_wdt_feed();

  out.cmd = UBUNTU_CMD_WDT_TIMEOUT;
  out.len = sizeof(int);
  memcpy(&out.data, &secs, out.len);
  ubuntu_ipc_cmd(&out, &in);

  mgos_wdt_enable();
  return;
}

void mgos_wdt_feed(void) {
  struct ubuntu_pipe_message out, in;

  out.cmd = UBUNTU_CMD_WDT;
  out.len = 0;
  ubuntu_ipc_cmd(&out, &in);
  return;
}

void mgos_wdt_enable(void) {
  struct ubuntu_pipe_message out, in;

  mgos_wdt_feed();

  out.cmd = UBUNTU_CMD_WDT_EN;
  out.len = 0;
  ubuntu_ipc_cmd(&out, &in);
  return;
}

void mgos_wdt_disable(void) {
  struct ubuntu_pipe_message out, in;

  out.cmd = UBUNTU_CMD_WDT_DIS;
  out.len = 0;
  ubuntu_ipc_cmd(&out, &in);
  return;
}

void ubuntu_ipc_ping(void) {
  struct ubuntu_pipe_message out, in;

  memset(&in, 0, sizeof(struct ubuntu_pipe_message));
  memset(&out, 0, sizeof(struct ubuntu_pipe_message));
  out.cmd = UBUNTU_CMD_PING;
  out.len = 4;
  memcpy(&out.data, "PING", out.len);
  ubuntu_ipc_cmd(&out, &in);
  LOG(LL_INFO, ("Sent: cmd=%u len=%u msg='%.*s'", out.cmd, out.len,
                (int) out.len, out.data));
  LOG(LL_INFO, ("Received: cmd=%u len=%u msg='%.*s'", in.cmd, in.len,
                (int) in.len, in.data));
}
