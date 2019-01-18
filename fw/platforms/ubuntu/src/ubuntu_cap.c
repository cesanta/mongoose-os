#include <sys/capability.h>
#include "mgos.h"

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
  bool chroot = false, bind = false;

  if (ubuntu_cap_have(CAP_NET_BIND_SERVICE)) {
    bind = true;
  } else {
    LOG(LL_ERROR, ("Lacking capability to bind ports <1024!"));
  }
  if (ubuntu_cap_have(CAP_SYS_CHROOT)) {
    chroot = true;
  } else {
    LOG(LL_ERROR, ("Lacking capability to chroot, using host filesystem, this is insecure!"));
  }
  return bind && chroot;
}
