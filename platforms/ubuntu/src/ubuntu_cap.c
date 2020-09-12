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

#include <sys/capability.h>
#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ubuntu.h"

// Note: Requires libcap-dev to be installed.
//
extern struct ubuntu_flags Flags;

static bool ubuntu_cap_have(cap_value_t c) {
  cap_t cap;
  pid_t pid;
  cap_flag_value_t f;

  if (!CAP_IS_SUPPORTED(c)) {
    return false;
  }

  pid = getpid();
  cap = cap_get_pid(pid);
  cap_get_flag(cap, c, CAP_EFFECTIVE, &f);
  cap_free(cap);
  return f == CAP_SET;
}

bool ubuntu_cap_init(void) {
  struct stat s;
  char conf_fn[200];

  if (!ubuntu_cap_have(CAP_NET_BIND_SERVICE)) {
    LOGM(LL_WARN,
         ("Lacking capability to bind ports <1024, continuing anyway"));
  }

  if (0 != chdir(Flags.chroot)) {
    LOGM(LL_ERROR, ("Cannot change to directory %s", Flags.chroot));
    return false;
  }
  LOGM(LL_INFO, ("Switched root to %s", Flags.chroot));
  snprintf(conf_fn, sizeof(conf_fn), "%s/conf0.json", Flags.chroot);
  if (0 != stat(conf_fn, &s)) {
    LOGM(LL_ERROR, ("Cannot stat %s", conf_fn));
    return false;
  }

  if (Flags.secure) {
    if (!ubuntu_cap_have(CAP_SYS_CHROOT)) {
      LOGM(LL_ERROR, ("Cannot chroot(), but secure mode is requested."));
      return false;
    }

    if (0 != chroot(Flags.chroot)) {
      LOGM(LL_ERROR, ("Cannot chroot to %s", Flags.chroot));
      return false;
    }
    if (chdir("/") != 0) {
      return false;
    }
    LOGM(LL_INFO, ("Setting chroot=%s", Flags.chroot));
  }

  // First, set the desired gid using --group flag.
  // If we're still running as gid=0, use the owner of the conf0.json file
  if ((Flags.gid != getgid()) && 0 != setgid(Flags.gid)) {
    LOGM(LL_ERROR, ("Cannot setgid to %d.", Flags.gid));
    return false;
  }

  if (getgid() == 0) {
    if (0 != setgid(s.st_gid)) {
      LOGM(LL_ERROR,
           ("Cannot setgid to the group of %s (%d).", conf_fn, s.st_gid));
      return false;
    }
    LOGM(LL_INFO, ("Set gid=%d based on %s.", getgid(), conf_fn));
  }

  // Now do the same for uid using --user flag.
  // If we're still running as uid=0, use the owner of the conf0.json file
  if ((Flags.uid != getuid()) && 0 != setuid(Flags.uid)) {
    LOG(LL_ERROR, ("Cannot setuid to %d.", Flags.uid));
    return false;
  }

  if (getuid() == 0) {
    if (0 != setuid(s.st_uid)) {
      LOGM(LL_ERROR,
           ("Cannot setuid to the owner of %s (%d).", conf_fn, s.st_uid));
      return false;
    }
    LOGM(LL_INFO, ("Set uid=%d based on %s.", getuid(), conf_fn));
  }

  // Still running as uid=0 or gid=0? Bail.
  if (getuid() == 0 || getgid() == 0) {
    LOGM(LL_ERROR, ("Refusing to run as uid=%d gid=%d.", getuid(), getgid()));
    return false;
  }
  LOGM(LL_INFO, ("pid=%d uid=%d gid=%d euid=%d egid=%d", getpid(), getuid(),
                 getgid(), geteuid(), getegid()));
  return true;
}
