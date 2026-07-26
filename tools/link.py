#!/usr/bin/python3

import os
import subprocess
import sys

# Wrapper for the linker that deduplicates static libs, collects them into a single group,
# and then run the modified link command.
#
# By default all static libs are wrapped in --whole-archive (historical
# behavior). If MGOS_LINK_WHOLE_ARCHIVE_LIBS is set (space-separated list of
# archive base names), only those archives are whole-archived; the rest are
# linked normally within the group. If MGOS_LINK_NO_WHOLE_ARCHIVE_LIBS is set,
# all archives except the listed basenames are whole-archived. The exclusion
# mode is useful with newer ESP-IDF where a small number of IDF archives contain
# intentionally-duplicated objects (e.g. wpa_supplicant eap_fast.c #includes
# eap_fast_pac.c) that collide when force-loaded.

whole_libs = os.environ.get("MGOS_LINK_WHOLE_ARCHIVE_LIBS")
if whole_libs is not None:
    whole_libs = set(whole_libs.split())

no_whole_libs = set(os.environ.get("MGOS_LINK_NO_WHOLE_ARCHIVE_LIBS", "").split())

args = [sys.argv[1]]
in_static_libs = False
whole = False


def set_whole(want):
    global whole
    if want != whole:
        args.append("-Wl,--whole-archive" if want else "-Wl,--no-whole-archive")
        whole = want


for arg in sys.argv[2:]:
    if not arg.endswith(".a"):
        if in_static_libs:
            set_whole(False)
            args.append("-Wl,--end-group")
            in_static_libs = False
        args.append(arg)
        continue
    if arg in args:
        continue
    if not in_static_libs:
        args.append("-Wl,--start-group")
        in_static_libs = True
    lib_name = os.path.basename(arg)
    if whole_libs is None:
        want_whole = lib_name not in no_whole_libs
    else:
        want_whole = lib_name in whole_libs
    set_whole(want_whole)
    args.append(arg)

res = subprocess.run(args)
sys.exit(res.returncode)
