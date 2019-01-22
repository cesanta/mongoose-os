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
#include "mgos.h"
#include "mgos_system.h"
#include "ubuntu.h"
#include "ubuntu_ipc.h"

static struct ubuntu_pipe s_pipe;

// Send a message (cmd, and inlen bytes) to the privileged (main) thread,
// then read back response and return it in *out, and the message length in *len.
bool ubuntu_ipc_cmd(const struct ubuntu_pipe_message *in, struct ubuntu_pipe_message *out) {
  size_t        _len;
  bool          ret = false;
  struct msghdr msg;
  struct iovec  iov[1];

  if (!in || !out) {
    return false;
  }

  memset(&msg, 0, sizeof(struct msghdr));
  iov[0].iov_base = (void *)in;
  iov[0].iov_len  = in->len + 2;
  msg.msg_iov     = iov;
  msg.msg_iovlen  = 1;

  mgos_rlock(s_pipe.lock);
  _len = sendmsg(s_pipe.mongoose_fd, &msg, 0);
  if (_len < 2) {
    LOG(LL_ERROR, ("Cannot write message"));
    goto exit;
  }

  iov[0].iov_base = (void *)out;
  iov[0].iov_len  = sizeof(struct ubuntu_pipe_message);
  msg.msg_iov     = iov;
  msg.msg_iovlen  = 1;
  _len            = recvmsg(s_pipe.mongoose_fd, &msg, 0);
  if (_len < 2) {
    LOG(LL_ERROR, ("Cannot read message"));
    goto exit;
  }
exit:
  mgos_runlock(s_pipe.lock);
  return ret;
}

static bool ubuntu_ipc_handle_open(const struct ubuntu_pipe_message *in, struct ubuntu_pipe_message *out) {
  int fd = -1;

  if (!in || !out) {
    return false;
  }

  if (0 == fnmatch("/dev/i2c-[0-9]*", (const char *)in->data, FNM_PATHNAME)) {
    fd = open((const char *)in->data, O_RDONLY);
  } else if (0 == fnmatch("/dev/spidev[0-9]*.[0-9]*", (const char *)in->data, FNM_PATHNAME)) {
    fd = open((const char *)in->data, O_RDONLY);
  } else {
    printf("Refusing to open '%s'", (char *)in->data);
  }

  memcpy(&out->data, &fd, sizeof(int));
  out->len = sizeof(int);
  return true;
}

bool ubuntu_ipc_handle(uint16_t timeout_ms) {
  fd_set                     rfds;
  struct timeval             tv;
  int                        retval;
  size_t                     _len;
  struct msghdr              msg;
  struct iovec               iov[1];
  struct ubuntu_pipe_message iovec_payload;

  FD_ZERO(&rfds);
  FD_SET(s_pipe.main_fd, &rfds);

  tv.tv_sec  = 0;
  tv.tv_usec = timeout_ms * 1000;

//  printf("Selecting for %u ms\n", timeout_ms);
  retval = select(FD_SETSIZE, &rfds, NULL, NULL, &tv);
  if (retval < 0) {
    perror("Cannot not select\n");
    return false;
  } else if (retval == 0) {
//    printf("No data within %u ms\n", timeout_ms);
    return true;
  }

  memset(&msg, 0, sizeof(struct msghdr));
  iov[0].iov_base = (void *)&iovec_payload;
  iov[0].iov_len  = sizeof(struct ubuntu_pipe_message);
  msg.msg_iov     = iov;
  msg.msg_iovlen  = 1;

  _len = recvmsg(s_pipe.main_fd, &msg, 0);
  if (_len < 2) {
    perror("Cannot read message\n");
    return false;
  }
  // printf("Received: cmd=%d len=%u msg='%.*s'\n", iovec_payload.cmd, iovec_payload.len, (int)iovec_payload.len, (char *)iovec_payload.data);

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

  case UBUNTU_CMD_OPEN:
    ubuntu_ipc_handle_open(&iovec_payload, &iovec_payload);
    // TODO(pim): Add control message here, see Stevens Unix Network
    // Programming page 428 functions Write_fd() and Read_fd()
    break;

  case UBUNTU_CMD_PING:
  default:
    iovec_payload.len = 5;
    memcpy(&iovec_payload.data, "PONG!", iovec_payload.len);
  }

  iov[0].iov_base = (void *)&iovec_payload;
  iov[0].iov_len  = iovec_payload.len + 2;
  msg.msg_iov     = iov;
  msg.msg_iovlen  = 1;
  _len            = sendmsg(s_pipe.main_fd, &msg, 0);
  if (_len < 2) {
    perror("Cannot write message data\n");
    return false;
  }
//  printf("Sent: cmd=%d len=%u msg='%.*s'\n", iovec_payload.cmd, iovec_payload.len, (int)iovec_payload.len, (char *)iovec_payload.data);
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
  s_pipe.main_fd     = fd[1];
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
