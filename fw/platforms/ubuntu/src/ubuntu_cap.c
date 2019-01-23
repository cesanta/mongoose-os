#include <sys/capability.h>
#include <sys/stat.h>
#include "mgos.h"
#include "ubuntu.h"

// Note: Requires libcap-dev to be installed.
//
extern struct ubuntu_flags Flags;

static bool ubuntu_cap_have(cap_value_t c) {
  cap_t            cap;
  pid_t            pid;
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
  char        conf_fn[200];

  if (!ubuntu_cap_have(CAP_NET_BIND_SERVICE)) {
    printf("ubuntu_cap_init: WARNING: Lacking capability to bind ports <1024, continuing anyway\n");
  }

  if (Flags.secure) {
    if (!ubuntu_cap_have(CAP_SYS_CHROOT)) {
      printf("Cannot chroot(), but secure mode is requested.\n");
      return false;
    }
    if (0 != chdir(Flags.chroot)) {
      printf("Cannot change directory %s\n", Flags.chroot);
      return false;
    }

    snprintf(conf_fn, sizeof(conf_fn), "%s/conf0.json", Flags.chroot);
    if (0 != stat(conf_fn, &s)) {
      printf("Cannot stat %s\n", conf_fn);
      return false;
    }

    if (0 != chroot(Flags.chroot)) {
      printf("Cannot chroot to %s\n", Flags.chroot);
      return false;
    }
    chdir("/");
    printf("ubuntu_cap_init: chroot=%s\n", Flags.chroot);
  }

  // First, set the desired gid using --group flag.
  // If we're still running as gid=0, use the owner of the conf0.json file
  if ((Flags.gid != getgid()) && 0 != setgid(Flags.gid)) {
    printf("Cannot setgid to %d.\n", Flags.gid);
    return false;
  }

  if (getgid() == 0) {
    if (0 != setgid(s.st_gid)) {
      printf("Cannot setgid to the group of %s (%d).\n", conf_fn, s.st_gid);
      return false;
    }
    printf("ubuntu_cap_init: Set gid=%d based on %s.\n", getgid(), conf_fn);
  }

  // Now do the same for uid using --user flag.
  // If we're still running as uid=0, use the owner of the conf0.json file
  if ((Flags.uid != getuid()) && 0 != setuid(Flags.uid)) {
    printf("Cannot setuid to %d.\n", Flags.uid);
    return false;
  }

  if (getuid() == 0) {
    if (0 != setuid(s.st_uid)) {
      printf("Cannot setuid to the owner of %s (%d).\n", conf_fn, s.st_uid);
      return false;
    }
    printf("ubuntu_cap_init: Set uid=%d based on %s.\n", getuid(), conf_fn);
  }

  // Still running as uid=0 or gid=0? Bail.
  if (getuid() == 0 || getgid() == 0) {
    printf("Refusing to run as uid=%d gid=%d.\n", getuid(), getgid());
    return false;
  }
  printf("ubuntu_cap_init: pid=%d uid=%d gid=%d euid=%d egid=%d\n", getpid(), getuid(), getgid(), geteuid(), getegid());
  return true;
}
