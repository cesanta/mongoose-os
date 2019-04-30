#!/usr/bin/env python3
#
# Copyright (c) 2014-2018 Cesanta Software Limited
# All rights reserved
#
# Licensed under the Apache License, Version 2.0 (the ""License"");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an ""AS IS"" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


#
# A simple tool to make RPC requests over UART.
#
# Examples:
#
#  $ mgrpc_uart.py /dev/ttyUSB0 Sys.GetInfo
#
#  $ mgrpc_uart.py /dev/ttyUSB0 Config.Get '{"key": "device.id"}'
#

import binascii
import json
import sys

import serial  # apt-get install python3-serial || pip3 install pyserial


class RPCError(Exception):
    pass


class RPCClient(object):

    def __init__(self, fd):
        self._fd = fd
        self._rpc_id_counter = 1


    def Call(self, method, params=None, wait_resp=True):
        frame = {
            "jsonrpc": "2.0",
            "method": method,
        }
        if params:
            frame["params"] = params
        if wait_resp:
            frame["id"] = self._rpc_id_counter
            self._rpc_id_counter += 1
        frame_json = json.dumps(frame)
        crc32 = binascii.crc32(bytes(frame_json, "utf-8"))
        frame_json += "%08x" % crc32
        #print(">> %s" % frame_json)
        self._fd.write(bytes(frame_json + "\n", "utf-8"))
        line = b""
        while wait_resp:
            b = self._fd.read(1)
            if b != b"\n":
                line += b
                continue
            done, r = self._ProcessLine(line.decode("utf-8"))
            if done:
                return r
            line = b""

    def _ProcessLine(self, line):
        line = line.strip()
        #print("<< %s" % line)
        if not line:
            return False, None
        if line[0] != "{" or len(line) < 10:
            return False, None
        cb = line.rindex("}")
        if cb < 0:
            return False, None
        payload = line[:cb+1]
        meta = line[cb+1:]
        if meta:
            fields = meta.split(",")
            exp_crc = int(fields[0], 16)
            crc = binascii.crc32(bytes(payload, "utf-8"))
            if crc != exp_crc:
                return False, None
        frame = json.loads(payload)
        if "result" in frame:
            return True, frame["result"]
        elif "error" in frame:
            code = frame["error"]["code"]
            message = frame["error"].get("message", "")
            e = RPCError("%d %s" % (code, message))
            e.code = code
            e.message = message
            raise e


if __name__ == "__main__":
    port, method = sys.argv[1], sys.argv[2]
    if len(sys.argv) == 4:
        params = json.loads(sys.argv[3])
    else:
        params = None
    with serial.Serial(port, baudrate=115200) as s:
        cl = RPCClient(s)
        try:
            print(cl.Call(method, params))
        except RPCError as e:
            print(e, file=sys.stderr)
            sys.exit(e.code)
