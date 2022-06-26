#!/usr/bin/python3

import subprocess
import sys

# Wrapper for the linker that deduplicates static libs, collects them into a single group,
# and then run the modified link command.

args = [sys.argv[1]]
in_static_libs = False
for arg in sys.argv[2:]:
    if not arg.endswith(".a"):
        if in_static_libs:
            args.append("-Wl,--end-group")
            args.append("-Wl,--no-whole-archive")
            in_static_libs = False
        args.append(arg)
        continue
    if arg in args:
        continue
    if not in_static_libs:
        args.append("-Wl,--start-group")
        args.append("-Wl,--whole-archive")
        in_static_libs = True
    args.append(arg)

res = subprocess.run(args)
sys.exit(res.returncode)
