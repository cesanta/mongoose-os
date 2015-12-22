#!/bin/bash
#
# This tool offerts a dead simple way to doing post mortem analysis
# of an ESP8266 crash.
#
# All you need is a file containing a dump of the serial log
# containing the core dump snippet:
#
# -------- Core Dump --------
# [base64 encoded lines]
# -------- End Core Dump --------
#
# And run:
#
#     tools/debug_coredump.sh <my_log_file> [build_out_elf_file]
#
# OTA enabled builds:
# -------------------
#
# OTA enabled builds produce 2 or more firmware images, built with different base addresses.
#
# When debugging a coredump of an OTA enabled firmware, you need to know which OTA slot
# produced the dump, otherwise GDB won't be able to resolve symbols and thus perform backtraces.
# This OTA slot is not currently encoded in the core dump, so you either have to know it upfront,
# or you can just perform one run, see the address of program counter, and deduce which slot you're debugging
# based on the address. The metadata.json contains the mapping between firmware slots and base addresses.
#
# The build out file must be a relative path pointing to a file located
# under the current directory. It defaults to smartjs.out.
# Example for OTA builds: smartjs.0x11000.out
#
# Note:
# -----
# OSX and Windows users: the log file should be under your Home directory
# otherwise docker-machine (or boot2docker) won't make it available to your
# docker VM.

SDK=docker.cesanta.com:5000/esp8266-build-oss:1.5.0-cesanta-r1

cd $(dirname $0)
BASEDIR=$(dirname $PWD)
DIR=$(dirname /cesanta${PWD##$BASEDIR})

LOG="$1"
OUT="${2:-build/smartjs.out}"

if [ -z "${LOG}" ]; then
    echo "usage: $0 console_log_file [build_out_elf_file]"
fi

# DIR /cesanta//smartjs/platforms/esp8266
# run core server in background and connect xt-gdb to it
exec docker run --rm -it -v /${BASEDIR}:/cesanta -v ${LOG}:/var/log/esp-console.log ${SDK} /bin/bash -c \
     "cd ${DIR} \
        ; tools/serve_core.py \
                               --irom firmware/0x11000.bin \
                               --iram firmware/0x00000.bin \
                               --rom tools/rom.bin \
                               /var/log/esp-console.log \
        & xt-gdb ${OUT} -ex 'target remote 127.0.0.1:1234' -ex 'set confirm off' -ex 'add-symbol-file tools/romsyms 0x40000000'"
