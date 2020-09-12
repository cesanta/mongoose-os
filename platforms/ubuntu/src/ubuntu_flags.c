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

#include <getopt.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>

#include "ubuntu.h"

extern struct ubuntu_flags Flags;

static void ubuntu_flags_default(void) {
  if (Flags.chroot) {
    free(Flags.chroot);
  }
  Flags.uid = getuid();
  Flags.gid = getgid();
  Flags.chroot = realpath("./build/fs/", NULL);

  Flags.secure = true;
  return;
}

static void ubuntu_flags_usage(char *progname) {
  printf("Usage:\n");
  printf(
      "  %s [--secure|--insecure] [-u|--user <user>] [-g|--group <group>] "
      "[-c|--chroot <dir>] [-h|--help]\n",
      basename(progname));
  printf("\n");
  printf(
      "  --user <uid> If running as root, this changes the userid to <uid> "
      "(must be a number) before starting Mongoose.\n");
  printf(
      "  --group <gid> If running as root, this changes the group to <group> "
      "(must be a number) before starting Mongoose.\n");
  printf(
      "  --chroot <dir> If running as root (or awarded cap_sys_chroot), this "
      "changes the root directory to <dir> before starting Mongoose.\n");
  printf("  --secure will fail if chroot is not possible (the default)\n");
  printf(
      "  --insecure will allow to run without changing user, group, chroot, "
      "but this is not advised!\n");
  printf("  --help prints this usage.\n");
  printf("\n");
  printf(
      "All application options (used by Mongoose itself) are to be set in "
      "mos.yml\n");
}

static uid_t ubuntu_flags_valid_user(char *u) {
  uid_t uid;

  if (!u) {
    return 0;
  }
  if (getuid() != 0) {
    printf("Cannot set user if not running as root\n");
    return 0;
  }
  uid = atoi(u);
  if (uid == 0) {
    printf(
        "Must provide a numeric uid greater than zero (you provided '%s').\n",
        u);
    return 0;
  }
  return uid;
}

static gid_t ubuntu_flags_valid_group(char *g) {
  uid_t gid;

  if (!g) {
    return 0;
  }
  if (getuid() != 0) {
    printf("Cannot set group if not running as root\n");
    return 0;
  }
  gid = atoi(g);
  if (gid == 0) {
    printf(
        "Must provide a numeric gid greater than zero (you provided '%s').\n",
        g);
    return 0;
  }
  return gid;
}

static bool ubuntu_flags_valid_dir(char *d) {
  DIR *dir;

  if (!d) {
    return false;
  }
  dir = opendir(d);
  if (!dir) {
    printf("Invalid directory (you provided '%s').\n", d);
    return false;
  }
  closedir(dir);
  return true;
}

bool ubuntu_flags_init(int argc, char **argv) {
  int c;
  bool ok = true;

  ubuntu_flags_default();

  for (;;) {
    static struct option long_options[] = {
        {"user", required_argument, 0, 'u'},
        {"group", required_argument, 0, 'g'},
        {"chroot", required_argument, 0, 'c'},
        {"secure", no_argument, &Flags.secure, 1},
        {"insecure", no_argument, &Flags.secure, 0},
        {"help", no_argument, 0, 'h'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}};
    int option_index = 0;

    c = getopt_long(argc, argv, "u:g:c:h", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1) {
      if (ok && Flags.chroot == NULL) {
        fprintf(stderr, "Invalid / missing --chroot\n");
        ok = false;
      }
      goto exit;
    }

    switch (c) {
      case 0:
        /* If this option set a flag, do nothing else now. */
        if (long_options[option_index].flag != 0) {
          break;
        }
        printf("option %s", long_options[option_index].name);
        if (optarg) {
          printf(" with arg %s", optarg);
        }
        printf("\n");
        break;

      case 'u': {
        uid_t uid;
        if ((uid = ubuntu_flags_valid_user(optarg))) {
          Flags.uid = uid;
        } else {
          ok = false;
          goto exit;
        }
        break;
      }

      case 'g': {
        gid_t gid;
        if ((gid = ubuntu_flags_valid_group(optarg))) {
          Flags.gid = gid;
        } else {
          ok = false;
          goto exit;
        }
        break;
      }

      case 'c':
        if (ubuntu_flags_valid_dir(optarg)) {
          if (Flags.chroot) {
            free(Flags.chroot);
          }
          Flags.chroot = realpath(optarg, NULL);
        } else {
          ok = false;
          goto exit;
        }
        break;

      case 'h':
      case '?':
      default:
        ok = false;
        goto exit;
    }
  }

exit:
  if (!ok) {
    ubuntu_flags_usage(argv[0]);
    return false;
  }
  return true;
}
