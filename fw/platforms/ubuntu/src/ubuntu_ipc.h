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
// char in[] = "Hello World!";
// char out[100];
// size_t len;
//
// ubuntu_ipc_cmd(UBUNTU_CMD_PING, in, sizeof(in), out, sizeof(out), &len);
// LOG(LL_INFO, ("Sent: cmd=%u len=%lu msg='%.*s'", UBUNTU_CMD_PING, sizeof(in), (int)sizeof(in), in));
// LOG(LL_INFO, ("Received: cmd=%u len=%lu msg='%.*s'", UBUNTU_CMD_PING, len, (int)len, out));
//

#include "mgos.h"

struct ubuntu_pipe {
  struct mgos_rlock_type *lock;
  int                     main_fd;
  int                     mongoose_fd;
};

enum ubuntu_pipe_cmd {
  UBUNTU_CMD_WDT=0,       // in=NULL, out=NULL
  UBUNTU_CMD_WDT_EN,      // in=NULL, out=NULL
  UBUNTU_CMD_WDT_DIS,     // in=NULL, out=NULL
  UBUNTU_CMD_WDT_TIMEOUT, // in=int *secs, out=NULL
  UBUNTU_CMD_PING,        // in=char *msg, out=char *msg
  UBUNTU_CMD_OPEN,        // in=char *path, out=int *fd;
};
typedef uint8_t ubuntu_pipe_cmd;

struct ubuntu_pipe_message {
  uint8_t cmd;
  uint8_t len;
  uint8_t data[256];
};

bool ubuntu_ipc_cmd(const struct ubuntu_pipe_message *in, struct ubuntu_pipe_message *out);
