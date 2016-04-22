#!/usr/bin/env python

import re
import subprocess
import sys

SYM_WHITELIST = set("""
user_init
__wrap_user_fatal_exception_handler
__cyg_profile_func_enter
__cyg_profile_func_exit
Cache_Read_Enable_New
pvPortMalloc
pvPortCalloc
pvPortZalloc
pvPortRealloc
system_restart_local
vPortFree
xPortGetFreeHeapSize
xPortWantedSizeAlign
""".strip().split())

def GetSymbols(fname):
    result = {}
    src = ''
    for l in subprocess.check_output("nm %s" % fname, shell=True).splitlines():
        m = re.match(r'(\S+):\s*$', l)
        if m:
            src = m.group(1)
            continue
        parts = l.strip().split()
        if len(parts) != 3: continue
        saddr, stype, sname = parts
        if stype == 'n' or stype == 'N': continue
        ssrc = 'ROM' if stype == 'A' else src
        result[sname] = ssrc
    return result

symcheck, app = sys.argv[1:]

sdk_syms = GetSymbols(symcheck)
app_syms = GetSymbols(app)

conflicting_syms = set(sdk_syms).intersection(set(app_syms)) - SYM_WHITELIST
if conflicting_syms:
    print >>sys.stderr, 'Conflicting symbols (defined in app and SDK):'
    for s in sorted(conflicting_syms):
        print '  %s (%s)' % (s, app_syms[s])
    sys.exit(1)
