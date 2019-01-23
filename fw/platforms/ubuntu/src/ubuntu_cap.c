#include <sys/capability.h>
#include "mgos.h"
#include "ubuntu.h"

// Note: Requires libcap-dev to be installed.
//
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
  if (!ubuntu_cap_have(CAP_NET_BIND_SERVICE)) {
    LOG(LL_ERROR, ("Lacking capability to bind ports <1024!"));
  }
  return true;
}

bool ubuntu_cap_chroot(const char *path) {
  if (!ubuntu_cap_have(CAP_SYS_CHROOT)) {
    LOG(LL_ERROR, ("Lacking capability to chroot, using host filesystem, this is insecure!"));
    return false;
  }
  if (0 != chdir(path)) {
    LOG(LL_ERROR, ("Cannot change directory %s", path));
    return false;
  }

  LOG(LL_INFO, ("Changing root to %s", path));
  if (0 != chroot(path)) {
    return false;
  }
  return 0 == chdir("/");
}
