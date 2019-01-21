#include <fnmatch.h>
#include "mgos.h"
#include "mgos_system.h"
#include "ubuntu.h"
#include "ubuntu_ipc.h"

static struct ubuntu_pipe s_pipe;

// Send a message (cmd, and inlen bytes) to the privileged (main) thread,
// then read back response and return it in *out, and the message length in *len.
bool ubuntu_ipc_cmd(const struct ubuntu_pipe_message *in, struct ubuntu_pipe_message *out) {
  size_t _len;
  bool   ret = false;

  if (!in || !out) {
    return false;
  }

  mgos_rlock(s_pipe.lock);
  _len = write(s_pipe.mongoose_fd, in, in->len + 2);
  if (_len < 2) {
    LOG(LL_ERROR, ("Cannot write message"));
    goto exit;
  }

  _len = read(s_pipe.mongoose_fd, out, sizeof(struct ubuntu_pipe_message));
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
  struct ubuntu_pipe_message in_msg, out_msg;

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

  memset(&in_msg, 0, sizeof(struct ubuntu_pipe_message));
  memset(&out_msg, 0, sizeof(struct ubuntu_pipe_message));

  _len = read(s_pipe.main_fd, &in_msg, sizeof(struct ubuntu_pipe_message));
  if (_len < 2) {
    perror("Cannot read message\n");
    return false;
  }
  // printf("Received: cmd=%d len=%u msg='%.*s'\n", in_msg.cmd, in_msg.len, (int)in_msg.len, (char *)in_msg.data);

  out_msg.cmd = in_msg.cmd;
  out_msg.len = 0;
  // Handle command
  switch (in_msg.cmd) {
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
    memcpy(&secs, &in_msg.data, sizeof(int));
    ubuntu_wdt_set_timeout(secs);
    break;
  }

  case UBUNTU_CMD_OPEN:
    ubuntu_ipc_handle_open(&in_msg, &out_msg);
    break;

  case UBUNTU_CMD_PING:
  default:
    out_msg.len = 5;
    memcpy(&out_msg.data, "PONG!", out_msg.len);
  }

  _len = write(s_pipe.main_fd, &out_msg, out_msg.len + 2);
  if (_len < 2) {
    perror("Cannot write message data\n");
    return false;
  }
//  printf("Sent: cmd=%d len=%u msg='%.*s'\n", out_msg.cmd, out_msg.len, (int)out_msg.len, (char *)out_msg.data);
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
