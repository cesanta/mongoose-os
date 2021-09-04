#!/usr/bin/python3
#
# A tool to compare firmware symbols and show differences.
# Shows size differences and section changes.
#
# Example:
#   tools/mgos_fw_diff.py --objdump-binary=xtensa-lx106-elf-objdump \
#       build.old/objs/*.elf build/objs/*.elf
#

import argparse
import collections
import itertools
import typing as t
import subprocess
import sys


class Sym(t.NamedTuple):
    name: str
    sect: str
    size: int


def load_syms(fn, objdump_binary):
    syms: t.Dict[str, Sym] = {}
    od_output = subprocess.check_output([objdump_binary, "-t", fn])
    for l in od_output.decode("utf-8").splitlines():
        parts = l.split()
        if len(parts) < 5 or parts[2] not in ("O", "F"):
            continue
        s = Sym(name=parts[-1], sect=parts[3], size=int(parts[4], 16))
        syms[s.name] = s
    return syms


def sign(v):
    return f"+{v}" if v > 0 else f"{v}"


def get_totals(d):
    return " ".join(f"{k}: {sign(v)}" for k, v in sorted(d.items(), key=lambda e: abs(e[1]), reverse=True))


def do_diff(old_binary, new_binary, objdump_binary, size_cutoff):
    syms1 = load_syms(old_binary, objdump_binary)
    syms2 = load_syms(new_binary, objdump_binary)

    additions: t.List[Sym] = []
    same: t.List[Sym] = []
    changes: t.List[t.Tuple[Sym, Sym]] = []
    removals: t.List[Sym] = []

    for name in sorted(set(itertools.chain(syms1.keys(), syms2.keys()))):
        s1 = syms1.get(name)
        s2 = syms2.get(name)
        if s1 and not s2:
            removals.append(s1)
        elif s2 and not s1:
            additions.append(s2)
        else:
            if s1 == s2:
                same.append(s1)
            else:
                changes.append((s1, s2))

    overall_size_diff: t.Dict[str, int] = collections.defaultdict(lambda: 0)

    print(f"Additions {len(additions)}:")
    additions_size: t.Dict[str, int] = collections.defaultdict(lambda: 0)
    for s in sorted(additions, key=lambda s: s.size, reverse=True):
        if s.size >= size_cutoff:
            print(f"  {s.name} {s.sect} +{s.size}")
        additions_size[s.sect] += s.size
        overall_size_diff[s.sect] += s.size

    print(f" Total additions: {get_totals(additions_size)}")
    print()
    print(f"Removals {len(removals)}:")
    removals_size: t.Dict[str, int] = collections.defaultdict(lambda: 0)
    for s in sorted(removals, key=lambda s: s.size, reverse=True):
        if s.size >= size_cutoff:
            print(f"  {s.name} {s.sect} -{s.size}")
        removals_size[s.sect] -= s.size
        overall_size_diff[s.sect] -= s.size
    print(f" Total removals: {get_totals(removals_size)}")
    print()
    print(f"Changes {len(changes)}:")
    changes_size: t.Dict[str, int] = collections.defaultdict(lambda: 0)
    for s1, s2 in sorted(changes, key=lambda ss: abs(ss[1].size - ss[0].size), reverse=True):
        size_diff = s2.size - s1.size
        if s1.sect == s2.sect:
            if s1.size >= size_cutoff or s2.size >= size_cutoff:
                print(f"  {s1.name} {s1.sect} {sign(size_diff)}")
        else:
            print(f"  {s1.name} {s1.sect}/{s2.sect} {sign(size_diff)}")
        changes_size[s1.sect] += size_diff
        overall_size_diff[s1.sect] += size_diff
    print(f" Total changes: {get_totals(changes_size)}")
    print()
    print(f"Overall changes: {get_totals(overall_size_diff)}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Firmware size difference tool", prog="mgos_fw_diff")
    parser.add_argument("--objdump-binary", default="objdump", help="objdump binary to use")
    parser.add_argument("--breakdown-size-cutoff", type=int, default=0, help="Do not print items less than this size")
    parser.add_argument("old_binary", nargs=1, help="Old ELF binary")
    parser.add_argument("new_binary", nargs=1, help="New ELF binary")
    args = parser.parse_args()
    do_diff(args.old_binary[0], args.new_binary[0], args.objdump_binary, args.breakdown_size_cutoff)

