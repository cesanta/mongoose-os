#!/bin/bash
#
# This tool offerts a dead simple way to doing post mortem analysis
# of an ESP8266 crash.
#
# All you need is a file containing a dump of the serial log
# containing the core dump snippet:
#
# -------- Core Dump --------
# [very long line]
# -------- End Core Dump --------
#
# And run:
#
#     tools/debug_coredump.sh <my_log_file>
#
# OSX and Windows users: the log file should be under your Home directory
# otherwise docker-machine (or boot2docker) won't make it available to your
# docker VM.

SDK=docker.cesanta.com:5000/esp8266-build-oss:1.3.0-cesanta

# The GDB distributed in our stable SDK image has an old gdb
# that doesn't still do a good job on stacktraces. However
# the latest one also has some problem and the image is big
# so we'll default with the old GDB which is good enough for the job
if [ "$1" == "--experimental" ]; then
    shift
    SDK=docker.cesanta.com:5000/esp8266-build-bleed:latest
fi

cd $(dirname $0)
V7DIR=$(dirname $(dirname $(dirname $(dirname $PWD))))
DIR=/cesanta/${PWD##$V7DIR}

LOG="$1"

if [ -z "${LOG}" ]; then
    echo "usage: $0 console_log_file"
fi

# run core server in background and connect xt-gdb to it
exec docker run --rm -it -v /${V7DIR}:/cesanta -v ${LOG}:/var/log/esp-console.log ${SDK} /bin/bash -c \
     "cd /cesanta/smartjs/platforms/esp8266/ \
        ; tools/serve_core.py 2>/dev/null \
                               --irom firmware/0x11000.bin \
                               --iram firmware/0x00000.bin \
                               --rom tools/rom.bin \
                               /var/log/esp-console.log \
        & xt-gdb build/smartjs.out -ex 'target remote 127.0.0.1:1234' -ex 'set confirm off' -ex 'add-symbol-file tools/romsyms 0x40000000'"
