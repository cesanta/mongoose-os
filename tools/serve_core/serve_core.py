#!/usr/bin/env python3

#
# usage: tools/serve_core.py build/fw/objs/fw.elf /tmp/console.log
#
# Then you can connect with gdb. The ESP8266 SDK image provides a debugger with
# reasonable support of lx106. Example invocation:
#
# docker run -v $PWD:/cesanta -ti \
#    docker.cesanta.com/esp8266-build-oss:latest \
#    xt-gdb /cesanta/fw/platforms/esp8266/build/fw.out \
#    -ex "target remote localhost:1234"
#
# If you run on OSX or windows, you have to put the IP of your host instead of
# localhost since gdb will run in a virtualmachine.

import argparse
import base64
import binascii
import ctypes
import json
import os
import re
import socketserver
import struct
import sys

import elftools.elf.elffile  # apt install python-pyelftools

parser = argparse.ArgumentParser(description='Serve ESP core dump to GDB')
parser.add_argument('--port', default=1234, type=int, help='listening port')
parser.add_argument('--rom', required=False, help='rom section')
parser.add_argument('--rom_addr', required=False, type=lambda x: int(x,16), help='rom map addr')
parser.add_argument('--debug', action='store_true', default=False)
parser.add_argument('--xtensa_addr_fixup', default=False, type=bool)
parser.add_argument('--target_descriptions', default='/opt/serve_core')
parser.add_argument('elf', help='Program executable')
parser.add_argument('log', help='serial log containing core dump snippet')

args = parser.parse_args()

START_DELIM = b'--- BEGIN CORE DUMP ---'
END_DELIM =   b'---- END CORE DUMP ----'


class FreeRTOSTask(object):

    def __init__(self, e):
        self.xHandle = e["h"]
        self.pcTaskName = e["n"]
        self.eCurrentState = e["st"]
        self.uxCurrentPriority = e["cpri"]
        self.uxBasePriority = e["bpri"]
        self.pxStackBase = e["sb"]
        self.pxTopOfStack = e["sp"]
        if "regs" in e:
            self.regs = base64.decodebytes(bytes(e["regs"]["data"], "ascii"))
        else:
            self.regs = None

    def __str__(self):
        return "0x%x '%s' pri %d/%d sp 0x%x (%d free)" % (
                self.xHandle, self.pcTaskName, self.uxCurrentPriority, self.uxBasePriority,
                self.pxTopOfStack, self.pxTopOfStack - self.pxStackBase)


class Core(object):
    def __init__(self, filename):
        self._dump = self._read(filename)
        self.mem = self._map_core(self._dump)
        if args.rom:
            self.mem.extend(self._map_firmware(args.rom_addr, args.rom))
        self.mem.extend(self._map_elf(args.elf))
        self.regs = base64.decodebytes(bytes(self._dump["REGS"]["data"], "ascii"))
        if "freertos" in self._dump:
            print("Dump contains FreeRTOS task info")
            self.tasks = dict((t["h"], FreeRTOSTask(t)) for t in self._dump["freertos"]["tasks"])
        else:
            self.tasks = {}
        self.target_features = self._dump.get("target_features")

    def get_cur_task(self):
        return self._dump.get("freertos", {}).get("cur", None)

    def _search_backwards(self, f, start_offset, pattern):
        offset = start_offset
        while True:
            offset = max(0, offset - 10000)
            f.seek(offset)
            data = f.read(min(10000, start_offset))
            pos = data.rfind(pattern)
            if pos >= 0:
                return offset + pos
            elif offset == 0:
                return -1
            offset += 5000

    def _read(self, filename):
        with open(filename, "rb") as f:
            f.seek(0, os.SEEK_END)
            size = f.tell()
            end_pos = self._search_backwards(f, f.tell(), END_DELIM)
            if end_pos == -1:
                print("Cannot find end delimiter:", END_DELIM, file=sys.stderr)
                sys.exit(1)
            start_pos = self._search_backwards(f, end_pos, START_DELIM)
            if start_pos == -1:
                print("Cannot find start delimiter:", START_DELIM, file=sys.stderr)
                sys.exit(1)
            start_pos += len(START_DELIM)
            print("Found core at %d - %d" % (start_pos, end_pos), file=sys.stderr)
            f.seek(start_pos)
            core_lines = []
            while True:
                l = f.readline().strip()
                if l == END_DELIM:
                    break
                core_lines.append(l.decode("ascii"))
            core_json = ''.join(core_lines)
            stripped = re.sub(r'(?im)\s+(\[.{1,40}\])?\s*', '', core_json)
            return json.loads(stripped)

    def _map_core(self, core):
        mem = []
        for k, v in list(core.items()):
            if not isinstance(v, dict) or k == 'REGS' or "addr" not in v:
                continue
            data = base64.decodebytes(bytes(v["data"], "ascii"))
            print("Mapping {0}: {1} @ {2:#02x}".format(k, len(data), v["addr"]), file=sys.stderr)
            if "crc32" in v:
                crc32 = ctypes.c_uint32(binascii.crc32(data))
                expected_crc32 = ctypes.c_uint32(v["crc32"])
                if crc32.value != expected_crc32.value:
                    print("CRC mismatch, section corrupted %s %s" % (crc32, expected_crc32), file=sys.stderr)
                    sys.exit(1)
            mem.append((v["addr"], v["addr"] + len(data), data))
        return mem

    def _map_firmware(self, addr, filename):
        with open(filename, "rb") as f:
            data = f.read()
            result = []
            i = 0
            magic, count = struct.unpack('<BB', data[i:i+2])
            if magic == 0xea and count == 0x04:
                # This is a V2 image, IRAM will be inside.
                (magic, count, f1, f2, entry, _, irom_len) = struct.unpack('<BBBBIII', data[i:i+16])
                print("Mapping IROM: {0} @ {1:#02x}".format(irom_len, addr), file=sys.stderr)
                result.append((addr, addr + irom_len, data[i:i+irom_len+16]))
                # The rest (IRAM) will be in the core.
            else:
                print("Mapping {0} at {1:#02x}".format(filename, addr), file=sys.stderr)
                result.append((addr, addr + len(data), data))
            return result

    def _map_elf(self, elf_file_name):
        result = []
        f = open(elf_file_name, "rb")
        ef = elftools.elf.elffile.ELFFile(f)
        for i, sec in enumerate(ef.iter_sections()):
            addr, size, off = sec["sh_addr"], sec["sh_size"], sec["sh_offset"]
            if addr > 0 and size > 0:
                print("Mapping {0} {1}: {2} @ {3:#02x}".format(elf_file_name, sec.name, size, addr), file=sys.stderr)
                f.seek(off)
                assert f.tell() == off
                data = f.read(size)
                assert len(data) == size
                result.append((addr, addr + size, data))
        return result

    def read(self, addr, size):
        for base, end, data in self.mem:
            if addr >= base and addr < end:
                return data[addr - base : addr - base + size]
        print("Unmapped addr", hex(addr), file=sys.stderr)
        return b"\0" * size


class GDBHandler(socketserver.BaseRequestHandler):
    def handle(self):
        self._core = core = Core(args.log)
        self._curtask = None
        print("Loaded core dump from last snippet in ", args.log, file=sys.stderr)

        while self.expect_packet_start():
            pkt = self.read_packet()
            if args.debug:
                print("<<", pkt, file=sys.stderr)
            if pkt == "?": # status -> trap
                self.send_str("S09")
            elif pkt == "g": # dump registers
                if self._curtask and self._curtask.regs:
                    # Dump specific task's registers
                    regs = self._curtask.regs
                else:
                    regs = core.regs
                self.send_str(self.encode_bytes(regs))
            elif pkt[0] == "G": # set registers
                core.regs = self.decode_bytes(pkt[1:])
                self.send_str("OK")
            elif pkt[0] == "m": # read memory
                addr, size = [int(n, 16) for n in pkt[1:].split(',')]
                if args.xtensa_addr_fixup and addr < 0x10000000 and addr > 0x80000:
                    print('fixup %08x' % addr, file=sys.stderr)
                    addr |= 0x40000000
                bs = core.read(addr, size)
                #if bs == "\0\0\0\0":
                #    bs = "\x01\0\0\0"
                #print >>sys.stderr, "<<", " ".join("{:02x}".format(ord(c)) for c in bs)
                self.send_str(self.encode_bytes(bs))
            elif pkt.startswith("Hg"):
                tid = int(pkt[2:], 16)
                self._curtask = core.tasks.get(tid)
                self.send_str("OK")
            elif pkt.startswith("Hc-1"):
                # cannot continue, this is post mortem debugging
                self.send_str("E01")
            elif pkt == "qC":
                t = core.get_cur_task()
                if t:
                    self.send_str("QC%016x" % t)
                else:
                    self.send_str("1")
            elif pkt == "qAttached":
                self.send_str("1")
            elif pkt == "qSymbol::":
                self.send_str("OK")
            elif pkt == "qfThreadInfo":
                if core.tasks:
                    self.send_str("m%s" % ",".join("%016x"  % t for t in core.tasks))
                else:
                    self.send_str("l")
            elif pkt == "qsThreadInfo":
                self.send_str("l")
            elif pkt.startswith("qThreadExtraInfo,"):
                self.send_thread_extra_info(int(pkt[17:], 16))
            elif pkt[0] == "T":
                tid = int(pkt[1:], 16)
                if tid in core.tasks:
                    self.send_str("OK")
                else:
                    self.send_str("ERR00")
            elif pkt == "D":
                self.send_str("OK")
            elif pkt in ("qTStatus", "qOffsets", "vMustReplyEmpty"):
                # silently ignore
                self.send_str("")
            elif pkt.startswith("qSupported"):
                features = []
                if self._core.target_features:
                    features.append("qXfer:features:read+")
                    print("Target features: %s" % self._core.target_features)
                else:
                    features.append("qXfer:features:read-")
                self.send_str(";".join(features))
            elif pkt.startswith("qXfer:features:read:"):
                self.send_file(pkt)
            else:
                print("Ignoring unknown command '%s'" % (pkt,), file=sys.stderr)
                self.send_str("")

        print("GDB closed the connection", file=sys.stderr)
        sys.exit(0)

    def send_file(self, pkt):
        _, _, _, fname, off_len = pkt.split(":")
        if fname == "target.xml":
            fname = self._core.target_features
        if "/" in fname:
            self.send_str("E00")
            return
        fname = os.path.join(args.target_descriptions, fname)
        off_s, length_s = off_len.split(",")
        off, length = int(off_s, 16), int(length_s, 16)
        try:
            if off == 0:
                print("Serving %s" % fname)
            with open(fname, "rb") as f:
                if f.seek(off) != off:
                    self.send_str("l")
                    return
                data = f.read(length)
                if len(data) == length:
                    self.send_str("m" + data.decode("ascii"))
                elif len(data) > 0:
                    self.send_str("l" + data.decode("ascii"))
                else:
                    self.send_str("l")
        except IOError as e:
            print("error reading %s, %d @ %d: %s" % (fname, length, off, e))
            self.send_str("E00")

    def encode_bytes(self, bs):
        return binascii.hexlify(bs).decode("ascii")

    def decode_bytes(self, s):
        return binascii.unhexlify(s)

    def send_ack(self):
        self.request.sendall(b"+");

    def send_nack(self):
        self.request.sendall(b"-");

    def send_str(self, s):
        if type(s) is bytes:
            s = s.decode("ascii")
        if args.debug:
            print(">>", s, file=sys.stderr)
        self.request.sendall("${0}#{1:02x}".format(s, self._checksum(s)).encode("ascii"))

    def _checksum(self, s):
        if type(s) is str:
            return sum(ord(i) for i in s) % 0x100
        else: # bytes
            return sum(s) % 0x100

    def expect_packet_start(self):
        return len(self.read_until('$')) > 0

    def read_packet(self):
        pkt = self.read_until('#')
        chk = b""
        chk += self.request.recv(1)
        chk += self.request.recv(1)
        if len(chk) != 2:
            return ""
        if int(chk, 16) != self._checksum(pkt):
            print("Bad checksum for {0}; got: {1} want: {2:02x}".format(pkt, chk, "want:", self._checksum(pkt)), file=sys.stderr)
            self.send_nack()
            return ""
        self.send_ack()
        return pkt.decode("ascii")

    def read_until(self, limit):
        buf = b""
        limit = bytes(limit, "ascii")
        while True:
            ch = self.request.recv(1)
            if len(ch) == 0: # eof
                return ""
            if ch == limit:
                return buf
            buf += ch

    def send_thread_extra_info(self, tid):
        task = self._core.tasks.get(tid)
        if task:
            self.send_str(binascii.hexlify(str(task).encode("ascii")))
        else:
            self.send_str(binascii.hexlify(("[Invalid task 0x%08x]" % tid).encode("ascii")))



class TCPServer(socketserver.TCPServer):
    allow_reuse_address = True

server = TCPServer(('0.0.0.0', args.port), GDBHandler)
print("Waiting for gdb on", args.port)
server.serve_forever()
