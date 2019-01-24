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

#pragma once

// This is a public header that (system) libraries can use to send IPC
// messages from the Mongoose thread (which is running in a chroot())
// to the main thread (which is not). This allows us to open files
// such as /dev/i2c-*, /dev/spidev*, /sys/class/gpio/*
//
// The main thread creates a socket pair (with ubuntu_ipc_init()) and
// passes the client file descriptor on to the Mongoose thread.
//
// Usage:
// #include "ubuntu_ipc.h"
//
//  struct ubuntu_pipe_message out, in;
//  out.cmd=UBUNTU_CMD_PING;
//  out.len=4;
//  memcpy(&out.data, "PING", out.len);
//  ubuntu_ipc_cmd(&out, &in);
//  LOG(LL_INFO, ("Sent: cmd=%u len=%u msg='%.*s'", out.cmd, out.len,
//  (int)out.len, out.data));
//  LOG(LL_INFO, ("Received: cmd=%u len=%u msg='%.*s'", in.cmd, in.len,
//  (int)in.len, in.data));

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct ubuntu_pipe {
  struct mgos_rlock_type *lock;
  int main_fd;
  int mongoose_fd;
};

enum ubuntu_pipe_cmd {
  UBUNTU_CMD_WDT = 0,      // in=NULL; out=NULL
  UBUNTU_CMD_WDT_EN,       // in=NULL; out=NULL
  UBUNTU_CMD_WDT_DIS,      // in=NULL; out=NULL
  UBUNTU_CMD_WDT_TIMEOUT,  // in=int *secs; out=NULL
  UBUNTU_CMD_PING,         // in=char *msg; out=char *msg
  UBUNTU_CMD_OPEN,         // in=char *path, int *flags; out=NULL (fd in cmsg)
};
typedef uint8_t ubuntu_pipe_cmd;

struct ubuntu_pipe_message {
  uint8_t cmd;
  uint8_t len;
  uint8_t data[256];
};

// Helper -- perform open() on the Main process, and return the filedescriptor.
int ubuntu_ipc_open(const char *pathname, int flags);

// Send a ping message from Mongoose to Main and back.
void ubuntu_ipc_ping(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
