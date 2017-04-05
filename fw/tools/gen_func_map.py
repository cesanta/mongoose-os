#!/usr/bin/env python
#
# Copyright (c) 2014-2017 Cesanta Software Limited
# All rights reserved
#
# This tool generates address -> function name mapping file from an ELF binary.
# The file consists of a sequence of records consisting of a varint-encoded address difference
# and prefix-compressed name.
# It is optimized for compact storage, not lookup efficiency.

import argparse
import struct
import subprocess
import sys

parser = argparse.ArgumentParser(description="Generate an address -> function map for a binary")
parser.add_argument("--objdump_binary", default="objdump", help="The objdump binary to use")
parser.add_argument("--output", default="", help="Output file, default - stdout")
parser.add_argument("binary", help="The binary to analyze")


def encode_varint(n):
    r = ""
    while True:
        b = n & 0x7f
        n >>= 7
        if n != 0:
            b |= 0x80
        r += chr(b)
        if n == 0:
            break
    return r


def common_prefix_length(s1, s2):
    i = 0
    while i < len(s1) and i < len(s2) and s1[i] == s2[i]:
        i += 1
    return i


if __name__ == "__main__":
    args = parser.parse_args()
    output = subprocess.check_output([args.objdump_binary, "-t", args.binary])
    funcs = {}
    for line in output.splitlines():
        parts = line.split()
        if len(parts) != 6 or parts[2] != "F":
            continue
        addr, name = long(parts[0], 16), parts[5]
        funcs[addr] = name
    if args.output != "":
        f = open(args.output, "w")
    else:
        f = sys.stdout
    with f:
        prev_addr, prev_name = 0, ""
        for addr, name in sorted(funcs.items()):
            a_diff = addr - prev_addr
            n_prefix_length = common_prefix_length(prev_name, name)
            n_suff = name[n_prefix_length:]
            f.write(encode_varint(a_diff))
            f.write(encode_varint(n_prefix_length))
            f.write(encode_varint(len(n_suff)))
            f.write(n_suff)
            prev_addr, prev_name = addr, name
