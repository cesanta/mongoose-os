#!/usr/bin/env python
#
# ESP8266 ROM Bootloader Utility
# https://github.com/themadinventor/esptool
#
# Copyright (C) 2014 Fredrik Ahlberg
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful, but WITHOUT 
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA 02110-1301 USA.

import argparse
import hashlib
import json
import math
import os
import serial
import struct
import subprocess
import sys
import tempfile
import time

class ESPROM(object):

    # These are the currently known commands supported by the ROM
    ESP_FLASH_BEGIN = 0x02
    ESP_FLASH_DATA  = 0x03
    ESP_FLASH_END   = 0x04
    ESP_MEM_BEGIN   = 0x05
    ESP_MEM_END     = 0x06
    ESP_MEM_DATA    = 0x07
    ESP_SYNC        = 0x08
    ESP_WRITE_REG   = 0x09
    ESP_READ_REG    = 0x0a

    # Maximum block sized for RAM and Flash writes, respectively.
    ESP_RAM_BLOCK   = 0x1800
    ESP_FLASH_BLOCK = 0x400

    # Default baudrate. The ROM auto-bauds, so we can use more or less whatever we want.
    ESP_ROM_BAUD    = 115200

    # First byte of the application image
    ESP_IMAGE_MAGIC = 0xe9

    # Initial state for the checksum routine
    ESP_CHECKSUM_MAGIC = 0xef

    # OTP ROM addresses
    ESP_OTP_MAC0    = 0x3ff00050
    ESP_OTP_MAC1    = 0x3ff00054

    # Sflash stub: an assembly routine to read from spi flash and send to host
    SFLASH_STUB = (
        "\x80\x3c\x00\x40\x1c\x4b\x00\x40\x21\x11\x00\x40\x00\x80" \
        "\xfe\x3f\xc1\xfb\xff\xd1\xf8\xff\x2d\x0d\x31\xfd\xff\x41\xf7\xff\x4a" \
        "\xdd\x51\xf9\xff\xc0\x05\x00\x21\xf9\xff\x31\xf3\xff\x41\xf5\xff\xc0" \
        "\x04\x00\x0b\xcc\x56\xec\xfd\x06\xff\xff\x00\x00",
        0x40100000, 0x4010001c)

    def __init__(self, port = 0, baud = ESP_ROM_BAUD):
        self._port = serial.Serial(port)
        # setting baud rate in a separate step is a workaround for
        # CH341 driver on some Linux versions (this opens at 9600 then
        # sets), shouldn't matter for other platforms/drivers. See
        # https://github.com/themadinventor/esptool/issues/44#issuecomment-107094446
        self._port.baudrate = baud

    """ Read bytes from the serial port while performing SLIP unescaping """
    def read(self, length = 1):
        b = ''
        while len(b) < length:
            c = self._port.read(1)
            if c == '\xdb':
                c = self._port.read(1)
                if c == '\xdc':
                    b = b + '\xc0'
                elif c == '\xdd':
                    b = b + '\xdb'
                else:
                    raise Exception('Invalid SLIP escape')
            else:
                b = b + c
        return b

    def read_packet(self):
        b = ''
        while True:
            c = self._port.read(1)
            if c != '\xc0':
                print 'Invalid head of packet (%s)' % repr(c)
                continue
            break
             #   raise Exception('Invalid head of packet (%s)' % repr(c))
        while True:
            c = self._port.read(1)
            if c == '\xc0':
                break
            if c == '\xdb':
                c = self._port.read(1)
                if c == '\xdc':
                    b = b + '\xc0'
                elif c == '\xdd':
                    b = b + '\xdb'
                else:
                    raise Exception('Invalid SLIP escape')
            else:
                b = b + c
        return b

    """ Write bytes to the serial port while performing SLIP escaping """
    def write(self, packet):
        buf = '\xc0'+(packet.replace('\xdb','\xdb\xdd').replace('\xc0','\xdb\xdc'))+'\xc0'
        self._port.write(buf)

    """ Calculate checksum of a blob, as it is defined by the ROM """
    @staticmethod
    def checksum(data, state = ESP_CHECKSUM_MAGIC):
        for b in data:
            state ^= ord(b)
        return state

    """ Send a request and read the response """
    def command(self, op = None, data = None, chk = 0):
        if op:
            # Construct and send request
            pkt = struct.pack('<BBHI', 0x00, op, len(data), chk) + data
            # print '=>', repr(pkt)
            self.write(pkt)

        # Read header of response and parse
        r = self._port.read(1)
        if r != '\xc0':
            raise Exception('Invalid head of packet (%s)' % repr(r))
        hdr = self.read(8)
        (resp, op_ret, len_ret, val) = struct.unpack('<BBHI', hdr)
        if resp != 0x01 or (op and op_ret != op):
            raise Exception('Invalid response')
        # print '<=', repr(hdr)

        # The variable-length body
        body = self.read(len_ret)

        # Terminating byte
        if self._port.read(1) != chr(0xc0):
            raise Exception('Invalid end of packet')

        return val, body

    """ Perform a connection test """
    def sync(self):
        self.command(ESPROM.ESP_SYNC, '\x07\x07\x12\x20'+32*'\x55')
        for i in xrange(7):
            self.command()

    """ Try connecting repeatedly until successful, or giving up """
    def connect(self):
        print 'Connecting...'

        for _ in xrange(4):
            # issue reset-to-bootloader:
            # RTS = either CH_PD or nRESET (both active low = chip in reset)
            # DTR = GPIO0 (active low = boot to flasher)
            self._port.setDTR(False)
            self._port.setRTS(True)
            time.sleep(0.05)
            self._port.setDTR(True)
            self._port.setRTS(False)
            time.sleep(0.05)
            self._port.setDTR(False)

            self._port.timeout = 0.3 # worst-case latency timer should be 255ms (probably <20ms)
            for _ in xrange(4):
                try:
                    self._port.flushInput()
                    self._port.flushOutput()
                    self.sync()
                    self._port.timeout = 5
                    return
                except:
                    time.sleep(0.05)
        raise Exception('Failed to connect')

    """ Read memory address in target """
    def read_reg(self, addr):
        res = self.command(ESPROM.ESP_READ_REG, struct.pack('<I', addr))
        if res[1] != "\0\0":
            raise Exception('Failed to read target memory')
        return res[0]

    """ Write to memory address in target """
    def write_reg(self, addr, value, mask, delay_us = 0):
        if self.command(ESPROM.ESP_WRITE_REG,
                struct.pack('<IIII', addr, value, mask, delay_us))[1] != "\0\0":
            raise Exception('Failed to write target memory')

    """ Start downloading an application image to RAM """
    def mem_begin(self, size, blocks, blocksize, offset):
        if self.command(ESPROM.ESP_MEM_BEGIN,
                struct.pack('<IIII', size, blocks, blocksize, offset))[1] != "\0\0":
            raise Exception('Failed to enter RAM download mode')

    """ Send a block of an image to RAM """
    def mem_block(self, data, seq):
        if self.command(ESPROM.ESP_MEM_DATA,
                struct.pack('<IIII', len(data), seq, 0, 0)+data, ESPROM.checksum(data))[1] != "\0\0":
            raise Exception('Failed to write to target RAM')

    """ Leave download mode and run the application """
    def mem_finish(self, entrypoint = 0):
        if self.command(ESPROM.ESP_MEM_END,
                struct.pack('<II', int(entrypoint == 0), entrypoint))[1] != "\0\0":
            raise Exception('Failed to leave RAM download mode')

    """ Start downloading to Flash (performs an erase) """
    def flash_begin(self, size, offset):
        old_tmo = self._port.timeout
        num_blocks = (size + ESPROM.ESP_FLASH_BLOCK - 1) / ESPROM.ESP_FLASH_BLOCK

        sectors_per_block = 16
        sector_size = 4096
        num_sectors = (size + sector_size - 1) / sector_size
        start_sector = offset / sector_size

        head_sectors = sectors_per_block - (start_sector % sectors_per_block)
        if num_sectors < head_sectors:
            head_sectors = num_sectors

        if num_sectors < 2 * head_sectors:
            erase_size = (num_sectors + 1) / 2 * sector_size
        else:
            erase_size = (num_sectors - head_sectors) * sector_size

        self._port.timeout = 10
        if self.command(ESPROM.ESP_FLASH_BEGIN,
                struct.pack('<IIII', erase_size, num_blocks, ESPROM.ESP_FLASH_BLOCK, offset))[1] != "\0\0":
            raise Exception('Failed to enter Flash download mode')
        self._port.timeout = old_tmo

    """ Write block to flash """
    def flash_block(self, data, seq):
        if self.command(ESPROM.ESP_FLASH_DATA,
                struct.pack('<IIII', len(data), seq, 0, 0)+data, ESPROM.checksum(data))[1] != "\0\0":
            raise Exception('Failed to write to target Flash')

    """ Leave flash mode and run/reboot """
    def flash_finish(self, reboot = False):
        pkt = struct.pack('<I', int(not reboot))
        if self.command(ESPROM.ESP_FLASH_END, pkt)[1] != "\0\0":
            raise Exception('Failed to leave Flash mode')

    """ Run application code in flash """
    def run(self, reboot = False):
        # Fake flash begin immediately followed by flash end
        self.flash_begin(0, 0)
        self.flash_finish(reboot)

    """ Read MAC from OTP ROM """
    def read_mac(self):
        mac0 = esp.read_reg(esp.ESP_OTP_MAC0)
        mac1 = esp.read_reg(esp.ESP_OTP_MAC1)
        if ((mac1 >> 16) & 0xff) == 0:
            oui = (0x18, 0xfe, 0x34)
        elif ((mac1 >> 16) & 0xff) == 1:
            oui = (0xac, 0xd0, 0x74)
        else:
            raise Exception("Unknown OUI")
        return oui + ((mac1 >> 8) & 0xff, mac1 & 0xff, (mac0 >> 24) & 0xff)

    """ Read SPI flash manufacturer and device id """
    def flash_id(self):
        self.flash_begin(0, 0)
        self.write_reg(0x60000240, 0x0, 0xffffffff)
        self.write_reg(0x60000200, 0x10000000, 0xffffffff)
        flash_id = esp.read_reg(0x60000240)
        self.flash_finish(False)
        return flash_id

    """ Read SPI flash """
    def flash_read(self, offset, size, count = 1):
        # Create a custom stub
        stub = struct.pack('<III', offset, size, count) + self.SFLASH_STUB[0]

        # Trick ROM to initialize SFlash
        self.flash_begin(0, 0)

        # Download stub
        self.mem_begin(len(stub), 1, len(stub), 0x40100000)
        self.mem_block(stub, 0)
        self.mem_finish(0x4010001c)

        # Fetch the data
        data = ''
        for _ in xrange(count):
            if self._port.read(1) != '\xc0':
                raise Exception('Invalid head of packet (sflash read)')

            data += self.read(size)

            if self._port.read(1) != chr(0xc0):
                raise Exception('Invalid end of packet (sflash read)')

        return data

    """ Abuse the loader protocol to force flash to be left in write mode """
    def flash_unlock_dio(self):
        # Enable flash write mode
        self.flash_begin(0, 0)
        # Reset the chip rather than call flash_finish(), which would have
        # write protected the chip again (why oh why does it do that?!)
        self.mem_begin(0,0,0,0x40100000)
        self.mem_finish(0x40000080)

    """ Perform a chip erase of SPI flash """
    def flash_erase(self):
        # Trick ROM to initialize SFlash
        self.flash_begin(0, 0)

        # This is hacky: we don't have a custom stub, instead we trick
        # the bootloader to jump to the SPIEraseChip() routine and then halt/crash
        # when it tries to boot an unconfigured system.
        self.mem_begin(0,0,0,0x40100000)
        self.mem_finish(0x40004984)

        # Yup - there's no good way to detect if we succeeded.
        # It it on the other hand unlikely to fail.

    def run_stub(self, stub, params, read_output=True):
        stub = dict(stub)
        stub['code'] = unhexify(stub['code'])
        if 'data' in stub:
            stub['data'] = unhexify(stub['data'])

        if stub['num_params'] != len(params):
            raise Exception('Stub requires %d params, %d provided'
                            % (stub['num_params'], len(args.params)))

        params = struct.pack('<' + ('I' * stub['num_params']), *params)
        pc = params + stub['code']

        # Upload
        self.mem_begin(len(pc), 1, len(pc), stub['params_start'])
        self.mem_block(pc, 0)
        if 'data' in stub:
            self.mem_begin(len(stub['data']), 1, len(stub['data']), stub['data_start'])
            self.mem_block(stub['data'], 0)
        self.mem_finish(stub['entry'])

        if read_output:
            print 'Stub executed, reading response:'
            while True:
                p = self.read_packet()
                print hexify(p)
                if p == '':
                    return


class ESPFirmwareImage:
    
    def __init__(self, filename = None):
        self.segments = []
        self.entrypoint = 0
        self.flash_mode = 0
        self.flash_size_freq = 0

        if filename is not None:
            f = file(filename, 'rb')
            (magic, segments, self.flash_mode, self.flash_size_freq, self.entrypoint) = struct.unpack('<BBBBI', f.read(8))
            
            # some sanity check
            if magic != ESPROM.ESP_IMAGE_MAGIC or segments > 16:
                raise Exception('Invalid firmware image')
        
            for i in xrange(segments):
                (offset, size) = struct.unpack('<II', f.read(8))
                if offset > 0x40200000 or offset < 0x3ffe0000 or size > 65536:
                    raise Exception('Suspicious segment 0x%x, length %d' % (offset, size))
                segment_data = f.read(size)
                if len(segment_data) < size:
                    raise Exception('End of file reading segment 0x%x, length %d (actual length %d)' % (offset, size, len(segment_data)))
                self.segments.append((offset, size, segment_data))

            # Skip the padding. The checksum is stored in the last byte so that the
            # file is a multiple of 16 bytes.
            align = 15-(f.tell() % 16)
            f.seek(align, 1)

            self.checksum = ord(f.read(1))

    def add_segment(self, addr, data):
        # Data should be aligned on word boundary
        l = len(data)
        if l % 4:
            data += b"\x00" * (4 - l % 4)
        if l > 0:
            self.segments.append((addr, len(data), data))

    def save(self, filename):
        f = file(filename, 'wb')
        f.write(struct.pack('<BBBBI', ESPROM.ESP_IMAGE_MAGIC, len(self.segments),
            self.flash_mode, self.flash_size_freq, self.entrypoint))

        checksum = ESPROM.ESP_CHECKSUM_MAGIC
        for (offset, size, data) in self.segments:
            f.write(struct.pack('<II', offset, size))
            f.write(data)
            checksum = ESPROM.checksum(data, checksum)

        align = 15-(f.tell() % 16)
        f.seek(align, 1)
        f.write(struct.pack('B', checksum))


class ELFFile:

    def __init__(self, name):
        self.name = name
        self.symbols = None

    def _fetch_symbols(self):
        if self.symbols is not None:
            return
        self.symbols = {}
        try:
            tool_nm = "xtensa-lx106-elf-nm"
            if os.getenv('XTENSA_CORE')=='lx106':
                tool_nm = "xt-nm"
            proc = subprocess.Popen([tool_nm, self.name], stdout=subprocess.PIPE)
        except OSError:
            print "Error calling "+tool_nm+", do you have Xtensa toolchain in PATH?"
            sys.exit(1)
        for l in proc.stdout:
            fields = l.strip().split()
            self.symbols[fields[2]] = int(fields[0], 16)

    def get_symbol_addr(self, sym):
        self._fetch_symbols()
        return self.symbols[sym]

    def get_entry_point(self):
        tool_readelf = "xtensa-lx106-elf-readelf"
        if os.getenv('XTENSA_CORE')=='lx106':
            tool_readelf = "xt-readelf"
        try:
            proc = subprocess.Popen([tool_readelf, "-h", self.name], stdout=subprocess.PIPE)
        except OSError:
            print "Error calling "+tool_readelf+", do you have Xtensa toolchain in PATH?"
            sys.exit(1)
        for l in proc.stdout:
            fields = l.strip().split()
            if fields[0] == "Entry":
                return int(fields[3], 0);

    def load_section(self, section):
        tool_objcopy = "xtensa-lx106-elf-objcopy"
        if os.getenv('XTENSA_CORE')=='lx106':
            tool_objcopy = "xt-objcopy"
        tmpsection = tempfile.mktemp(suffix=".section")
        try:
            subprocess.check_call([tool_objcopy, "--only-section", section, "-Obinary", self.name, tmpsection])
            with open(tmpsection, "rb") as f:
                data = f.read()
        finally:
            os.remove(tmpsection)
        return data


class CesantaFlasher(object):
    # This is "wrapped" stub_flasher.c, to  be loaded using run_stub.
    FLASHER_STUB = """
    {"code_start": 1074790404, "code": "080000601C000060000000601000006031FCFF7\
1FCFF81FCFFC02000680332D210C020004807404074DCC48608005823C0200098081BA5A9239245\
0058031B555903582337350129230B446604DFC6F3FF21EEFFC0200069020DF0000000010078480\
040004A0040B449004012C1F0C921D911E901DD0209312020B4ED033C2C56C2073020B43C3C5642\
0701F5FFC000003C4C569206CD0EEADD860300202C4101F1FFC0000056A204C2DCF0C02DC0CC6CC\
AE2D1EAFF0606002030F456D3FD86FBFF00002020F501E8FFC00000EC82D0CCC0C02EC0C73DEB2A\
DC460300202C4101E1FFC00000DC42C2DCF0C02DC056BCFEC602003C5C8601003C6C4600003C7C0\
8312D0CD811C821E80112C1100DF0000C100000140010400C000060741000006410000080100000\
8C10000084100000881000009010000018980040880F0040A80F0040349800404C4A0040740F004\
0800F0040980F00400099004012C1E091F5FFC961CD0221EFFFE941F9310971D9519011C01A2239\
02E2D1100C02226E1D21E4FF31E9FF2AF11A332D0F42630001EAFFC00000C030B43C2256A31621E\
1FF1A2228022030B43C3256B31501ADFFC00000DD023C4256ED1431D6FF4D010C52D90E192E126E\
0101DDFFC0000021D2FF32A101C020004802303420C0200039022C0201D7FFC0000046330000003\
1CDFF1A333803D023C03199FF27B31ADC7F31CBFF1A3328030198FFC0000056C20E2193FF2ADD06\
0E000031C6FF1A3328030191FFC0000056820DD2DD10460800000021BEFF1A2228029CE231BCFFC\
020F51A33290331BBFFC02C411A332903C0F0F4222E1D22D204273D9332A3FFC02000280E27B3F7\
21ABFF381E1A2242A40001B5FFC00000381E2D0C42A40001B3FFC0000056120801B2FFC00000C02\
000280EC2DC0422D2FCC02000290E01ADFFC00000222E1D22D204226E1D281E22D204E7B204291E\
860000126E012198FF32A0042A210543003198FF222E1D1A33380337B202C6D6FF2C02019FFFC00\
0002191FF318CFF1A223A31019CFFC00000218DFF1C031A220540000C02060300003C528601003C\
624600003C72918BFF9A110871C861D851E841F83112C1200DF00010000058100000B01000001C4\
B0040803C004012C1E0C96191FBFFCD0331F8FFD951E9410971F931DD029011C0ED045C22473370\
21F3FF20F180F02F200177FFC00000C60F00000175FFC00000FD0CC7BE01FD0E2D0D3D014D0F01E\
CFFC000008C425C32460F000021E6FF3D011A224D0F016DFFC000002D013D0F01E5FFC00000FADD\
F0CCC022D11056ACFB31DDFF303180016AFFC0000022D1101C0301DCFFC000002D0C91D8FF9A110\
871C861D851E841F83112C1200DF00000C0100000D010000012C1E091FEFFC961D951E9410971F9\
31CD039011C0ED02DD0431C8FF9C1422A06247B302062D0021F4FF1A22490286010021F1FF1A223\
90221C2FF2AF12D0F0146FFC00000461C0022D1100143FFC0000021E9FFFD0C1A222802C7B20621\
E6FF1A22F8022D0E3D014D0F01B7FFC000008C5222A063C618000021B1FF3D01102280F04F20013\
8FFC00000AC7D22D1103D014D0F0134FFC0000021AAFF32D1101022800135FFC0000021A7FF1C03\
1A2201A7FFC00000FAEEF0CCC056ACF821A1FF31A0FF1A223A31012CFFC00000219DFF1C031A220\
19EFFC000002D0C91C8FF9A110871C861D851E841F83112C1200DF0000200600000001040020060\
FFFFFF0012C1E00C02290131FAFF21FAFF026107C961C02000226300C02000C80320CC10564CFF2\
1F5FFC02000380221F4FF20231029010C432D010185FFC0000008712D0CC86112C1200DF00080FE\
3F8449004012C1D0C9A109B17CFC22C1110C13C51C00261202463000220111C24110B68202462B0\
031F5FF3022A02802A002002D011C03851A0066820B2801322101C5AFFF060700003C1286050000\
10212032A01085180066A20F22210038114821C5BCFF224110861A004C1206FDFF10212032A0108\
5160066A20C2221003811482105D8FFC6F6FF5C1286F5FF0010212032A01085140066A20D222100\
3811482105E1FF06EFFF0022A06146EDFF45F0FFC6EBFF000001D2FFC0000006E9FF000C0222411\
00C1322C110C50F00220111060600000022C1100C13C50E0022011132C2FA303074B6230206C8FF\
08B1C8A112C1300DF0000000000010404F484149007519031027000000110040A8100040BC0F004\
0583F0040CC2E00401CE20040D83900408000004021F4FF12C1E0C961C80221F2FF097129010C02\
D951C91101F4FFC0000001F3FFC00000AC2C22A3E801F2FFC0000021EAFFC031412A233D0C01EFF\
FC000003D0222A00001EDFFC00000C1E4FF2D0C01E8FFC000002D0132A004450400C5E7FFDD022D\
0C01E3FFC00000666D1F4B2131DCFF4600004B22C0200048023794F531D9FFC0200039023DF0860\
1000001DCFFC000000871C861D85112C1200DF000000012C1F0026103010CFFC00000083112C110\
0DF000643B004012C1D0E98109B1C9A1D991F97129013911E2A0C001FAFFC00000CD02E792F40C0\
DE2A0C0F2A0DB860D00000001F4FFC00000204220E71240F7921C22610201EFFFC0000052A0DC48\
2157120952A0DD571205460500004D0C3801DA234242001BDD3811379DC5C6000000000C0DC2A0C\
001E3FFC00000C792F608B12D0DC8A1D891E881F87112C1300DF00000", \
"entry": 1074792024, "num_params": 1, "params_start": 1074790400, \
"data": "620510407E0510409F051040BE051040DE051040E6051040F0051040F0051040", \
"data_start": 1073643520}"""

    CMD_FLASH_WRITE = 1
    CMD_BOOT_FW = 6
    DEFAULT_FLASH_BAUD = 921600   # 0 means do not change baud rate

    def __init__(self, esp, baud_rate):
        if baud_rate > 0:
            print 'Running Cesanta flasher (speed %d)...' % baud_rate
        else:
            print 'Running Cesanta flasher...'
        self._esp = esp
        esp.run_stub(json.loads(self.FLASHER_STUB), [flash_baud], read_output=False)
        if flash_baud > 0:
            esp._port.baudrate = flash_baud
        # Read the greeting.
        p = esp.read_packet()
        if p != 'OHAI':
            raise Exception('Faild to connect to the flasher (got %s)' % hexify(p))

    def flash_write(self, addr, data):
        assert addr % 4096 == 0, 'Address must be sector-aligned'
        assert len(data) % 4096 == 0, 'Length must be sector-aligned'
        sys.stdout.write('Writing %d @ 0x%x... ' % (len(data), addr))
        self._esp.write(struct.pack('<B', self.CMD_FLASH_WRITE))
        self._esp.write(struct.pack('<III', addr, len(data), 1))
        num_sent, num_written = 0, 0
        write_size = 1024
        while num_written < len(data):
            p = self._esp.read_packet()
            if len(p) == 4:
                num_written = struct.unpack('<I', p)[0]
            elif len(p) == 1:
                status_code = struct.unpack('<B', p)[0]
                raise Exception('Write failure, status: %x' % status_code)
            else:
                raise Exception('Unexpected packet while writing: %s' % hexify(p))
            progress = '%d (%d %%)' % (num_written, num_written * 100.0 / len(data))
            sys.stdout.write(progress + '\b' * len(progress))
            sys.stdout.flush()
            while num_sent - num_written < 3072:
                self._esp._port.write(data[num_sent:num_sent+1024])
                num_sent += 1024
        p = self._esp.read_packet()
        if len(p) != 16:
            raise Exception('Expected digest, got: %s' % hexify(p))
        digest = hexify(p).upper()
        expected_digest = hashlib.md5(data).hexdigest().upper()
        print
        if digest != expected_digest:
            raise Exception('Digest mismatch: expected %s, got %s' % (expected_digest, digest))
        p = self._esp.read_packet()
        if len(p) != 1:
            raise Exception('Expected status, got: %s' % hexify(p))
        status_code = struct.unpack('<B', p)[0]
        if status_code != 0:
            raise Exception('Write failure, status: %x' % status_code)

    def boot_fw(self):
        self._esp.write(struct.pack('<B', self.CMD_BOOT_FW))
        p = self._esp.read_packet()
        if len(p) != 1:
            raise Exception('Expected status, got: %s' % hexify(p))
        status_code = struct.unpack('<B', p)[0]
        if status_code != 0:
            raise Exception('Boot failure, status: %x' % status_code)



def arg_auto_int(x):
    return int(x, 0)

def div_roundup(a, b):
    """ Return a/b rounded up to nearest integer,
    equivalent result to int(math.ceil(float(int(a)) / float(int(b))), only
    without possible floating point accuracy errors.
    """
    return (int(a) + int(b) - 1) / int(b)


def hexify(s):
    return ''.join('%02X' % ord(c) for c in s)


def unhexify(hs):
    s = ''
    for i in range(0, len(hs) - 1, 2):
        s += chr(int(hs[i] + hs[i+1], 16))
    return s


# Added by Cesanta (rojer).
def wrap_stub(args):
    e = ELFFile(args.input)

    stub = {
        'params_start': e.get_symbol_addr('_params_start'),
        'code': e.load_section('.code'),
        'code_start': e.get_symbol_addr('_code_start'),
        'entry': e.get_symbol_addr(args.entry),
    }
    data = e.load_section('.data')
    if len(data) > 0:
        stub['data'] = data
        stub['data_start'] = e.get_symbol_addr('_data_start')
    params_len = e.get_symbol_addr('_params_end') - stub['params_start']
    if params_len % 4 != 0:
        raise Exception('Params must be dwords')
    stub['num_params'] = params_len / 4

    # Pad code with NOPs to mod 4.
    if len(stub['code']) % 4 != 0:
        stub['code'] += (4 - (len(stub['code']) % 4)) * '\0'

    print >>sys.stderr, (
        'Stub params: %d @ 0x%08x, code: %d @ 0x%08x, data: %d @ 0x%08x, entry: %s @ 0x%x' % (
            params_len, stub['params_start'],
            len(stub['code']), stub['code_start'],
            len(stub.get('data', '')), stub.get('data_start', 0),
            args.entry, stub['entry']))

    jstub = dict(stub)
    jstub['code'] = hexify(stub['code'])
    if 'data' in stub:
        jstub['data'] = hexify(stub['data'])
    return stub, jstub


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description = 'ESP8266 ROM Bootloader Utility', prog = 'esptool')

    parser.add_argument(
            '--port', '-p',
            help = 'Serial port device',
            default = '/dev/ttyUSB0')

    parser.add_argument(
            '--baud', '-b',
            help = 'Serial port baud rate',
            type = arg_auto_int,
            default = ESPROM.ESP_ROM_BAUD)

    subparsers = parser.add_subparsers(
            dest = 'operation',
            help = 'Run esptool {command} -h for additional help')

    parser_load_ram = subparsers.add_parser(
            'load_ram',
            help = 'Download an image to RAM and execute')
    parser_load_ram.add_argument('filename', help = 'Firmware image')

    parser_dump_mem = subparsers.add_parser(
            'dump_mem',
            help = 'Dump arbitrary memory to disk')
    parser_dump_mem.add_argument('address', help = 'Base address', type = arg_auto_int)
    parser_dump_mem.add_argument('size', help = 'Size of region to dump', type = arg_auto_int)
    parser_dump_mem.add_argument('filename', help = 'Name of binary dump')

    parser_read_mem = subparsers.add_parser(
            'read_mem',
            help = 'Read arbitrary memory location')
    parser_read_mem.add_argument('address', help = 'Address to read', type = arg_auto_int)

    parser_write_mem = subparsers.add_parser(
            'write_mem',
            help = 'Read-modify-write to arbitrary memory location')
    parser_write_mem.add_argument('address', help = 'Address to write', type = arg_auto_int)
    parser_write_mem.add_argument('value', help = 'Value', type = arg_auto_int)
    parser_write_mem.add_argument('mask', help = 'Mask of bits to write', type = arg_auto_int)

    parser_write_flash = subparsers.add_parser(
            'write_flash',
            help = 'Write a binary blob to flash')
    parser_write_flash.add_argument('addr_filename', nargs = '+', help = 'Address and binary file to write there, separated by space')
    parser_write_flash.add_argument('--flash_freq', '-ff', help = 'SPI Flash frequency',
            choices = ['40m', '26m', '20m', '80m'], default = '40m')
    parser_write_flash.add_argument('--flash_mode', '-fm', help = 'SPI Flash mode',
            choices = ['qio', 'qout', 'dio', 'dout'], default = 'qio')
    parser_write_flash.add_argument('--flash_size', '-fs', help = 'SPI Flash size in Mbit',
            choices = ['4m', '2m', '8m', '16m', '32m', '16m-c1', '32m-c1', '32m-c2'], default = '4m')
    parser_write_flash.add_argument('--flash_baud', help = 'Baud rate to use while flashing', type = arg_auto_int)

    parser_run = subparsers.add_parser(
            'run',
            help = 'Run application code in flash')

    parser_image_info = subparsers.add_parser(
            'image_info',
            help = 'Dump headers from an application image')
    parser_image_info.add_argument('filename', help = 'Image file to parse')

    parser_make_image = subparsers.add_parser(
            'make_image',
            help = 'Create an application image from binary files')
    parser_make_image.add_argument('output', help = 'Output image file')
    parser_make_image.add_argument('--segfile', '-f', action = 'append', help = 'Segment input file') 
    parser_make_image.add_argument('--segaddr', '-a', action = 'append', help = 'Segment base address', type = arg_auto_int) 
    parser_make_image.add_argument('--entrypoint', '-e', help = 'Address of entry point', type = arg_auto_int, default = 0)

    parser_elf2image = subparsers.add_parser(
            'elf2image',
            help = 'Create an application image from ELF file')
    parser_elf2image.add_argument('input', help = 'Input ELF file')
    parser_elf2image.add_argument('--output', '-o', help = 'Output filename prefix', type = str)
    parser_elf2image.add_argument('--flash_freq', '-ff', help = 'SPI Flash frequency',
            choices = ['40m', '26m', '20m', '80m'], default = '40m')
    parser_elf2image.add_argument('--flash_mode', '-fm', help = 'SPI Flash mode',
            choices = ['qio', 'qout', 'dio', 'dout'], default = 'qio')
    parser_elf2image.add_argument('--flash_size', '-fs', help = 'SPI Flash size in Mbit',
            choices = ['4m', '2m', '8m', '16m', '32m', '16m-c1', '32m-c1', '32m-c2'], default = '4m')

    parser_read_mac = subparsers.add_parser(
            'read_mac',
            help = 'Read MAC address from OTP ROM')

    parser_flash_id = subparsers.add_parser(
            'flash_id',
            help = 'Read SPI flash manufacturer and device ID')

    parser_read_flash = subparsers.add_parser(
            'read_flash',
            help = 'Read SPI flash content')
    parser_read_flash.add_argument('address', help = 'Start address', type = arg_auto_int)
    parser_read_flash.add_argument('size', help = 'Size of region to dump', type = arg_auto_int)
    parser_read_flash.add_argument('filename', help = 'Name of binary dump')

    parser_erase_flash = subparsers.add_parser(
            'erase_flash',
            help = 'Perform Chip Erase on SPI flash')

    parser_wrap_stub = subparsers.add_parser(
            'wrap_stub',
            help = 'Wrap stub and output a JSON object')
    parser_wrap_stub.add_argument('input')
    parser_wrap_stub.add_argument('--entry', default='stub_main')

    parser_run_stub = subparsers.add_parser(
            'run_stub',
            help = 'Run stub on a device')
    parser_run_stub.add_argument('--entry', default='stub_main')
    parser_run_stub.add_argument('input')
    parser_run_stub.add_argument('params', nargs='*', type=arg_auto_int)

    args = parser.parse_args()

    # Create the ESPROM connection object, if needed
    esp = None
    if args.operation not in ('image_info','make_image','elf2image','wrap_stub'):
        esp = ESPROM(args.port, args.baud)
        esp.connect()

    # Do the actual work. Should probably be split into separate functions.
    if args.operation == 'load_ram':
        image = ESPFirmwareImage(args.filename)

        print 'RAM boot...'
        for (offset, size, data) in image.segments:
            print 'Downloading %d bytes at %08x...' % (size, offset),
            sys.stdout.flush()
            esp.mem_begin(size, div_roundup(size, esp.ESP_RAM_BLOCK), esp.ESP_RAM_BLOCK, offset)

            seq = 0
            while len(data) > 0:
                esp.mem_block(data[0:esp.ESP_RAM_BLOCK], seq)
                data = data[esp.ESP_RAM_BLOCK:]
                seq += 1
            print 'done!'

        print 'All segments done, executing at %08x' % image.entrypoint
        esp.mem_finish(image.entrypoint)

    elif args.operation == 'read_mem':
        print '0x%08x = 0x%08x' % (args.address, esp.read_reg(args.address))

    elif args.operation == 'write_mem':
        esp.write_reg(args.address, args.value, args.mask, 0)
        print 'Wrote %08x, mask %08x to %08x' % (args.value, args.mask, args.address)

    elif args.operation == 'dump_mem':
        f = file(args.filename, 'wb')
        for i in xrange(args.size/4):
            d = esp.read_reg(args.address+(i*4))
            f.write(struct.pack('<I', d))
            if f.tell() % 1024 == 0:
                print '\r%d bytes read... (%d %%)' % (f.tell(), f.tell()*100/args.size),
                sys.stdout.flush()
        print 'Done!'

    elif args.operation == 'write_flash':
        assert len(args.addr_filename) % 2 == 0

        flash_mode = {'qio':0, 'qout':1, 'dio':2, 'dout': 3}[args.flash_mode]
        flash_size_freq = {'4m':0x00, '2m':0x10, '8m':0x20, '16m':0x30, '32m':0x40, '16m-c1': 0x50, '32m-c1':0x60, '32m-c2':0x70}[args.flash_size]
        flash_size_freq += {'40m':0, '26m':1, '20m':2, '80m': 0xf}[args.flash_freq]
        flash_info = struct.pack('BB', flash_mode, flash_size_freq)

        flash_baud = args.flash_baud and args.flash_baud or CesantaFlasher.DEFAULT_FLASH_BAUD
        flasher = CesantaFlasher(esp, flash_baud)

        while args.addr_filename:
            address = int(args.addr_filename[0], 0)
            filename = args.addr_filename[1]
            args.addr_filename = args.addr_filename[2:]
            image = file(filename, 'rb').read()
            # Fix sflash config data.
            if address == 0 and image[0] == '\xe9':
                image = image[0:2] + flash_info + image[4:]
            # Pad to sector size, which is the minimum unit of writing (erasing really).
            if len(image) % 4096 != 0:
                image += '\xff' * (4096 - (len(image) % 4096))
            t = time.time()
            flasher.flash_write(address, image)
            t = time.time() - t
            print ('\rWrote %d bytes at 0x%08x in %.1f seconds (%.1f kbit/s)...'
                   % (len(image), address, t, len(image) / t * 8 / 1000))
        print 'Leaving...'
        flasher.boot_fw()

    elif args.operation == 'run':
        esp.run()

    elif args.operation == 'image_info':
        image = ESPFirmwareImage(args.filename)
        print ('Entry point: %08x' % image.entrypoint) if image.entrypoint != 0 else 'Entry point not set'
        print '%d segments' % len(image.segments)
        print
        checksum = ESPROM.ESP_CHECKSUM_MAGIC
        for (idx, (offset, size, data)) in enumerate(image.segments):
            print 'Segment %d: %5d bytes at %08x' % (idx+1, size, offset)
            checksum = ESPROM.checksum(data, checksum)
        print
        print 'Checksum: %02x (%s)' % (image.checksum, 'valid' if image.checksum == checksum else 'invalid!')

    elif args.operation == 'make_image':
        image = ESPFirmwareImage()
        if len(args.segfile) == 0:
            raise Exception('No segments specified')
        if len(args.segfile) != len(args.segaddr):
            raise Exception('Number of specified files does not match number of specified addresses')
        for (seg, addr) in zip(args.segfile, args.segaddr):
            data = file(seg, 'rb').read()
            image.add_segment(addr, data)
        image.entrypoint = args.entrypoint
        image.save(args.output)

    elif args.operation == 'elf2image':
        if args.output is None:
            args.output = args.input + '-'
        e = ELFFile(args.input)
        image = ESPFirmwareImage()
        image.entrypoint = e.get_entry_point()
        for section, start in ((".text", "_text_start"), (".data", "_data_start"), (".rodata", "_rodata_start")):
            data = e.load_section(section)
            image.add_segment(e.get_symbol_addr(start), data)

        image.flash_mode = {'qio':0, 'qout':1, 'dio':2, 'dout': 3}[args.flash_mode]
        image.flash_size_freq = {'4m':0x00, '2m':0x10, '8m':0x20, '16m':0x30, '32m':0x40, '16m-c1': 0x50, '32m-c1':0x60, '32m-c2':0x70}[args.flash_size]
        image.flash_size_freq += {'40m':0, '26m':1, '20m':2, '80m': 0xf}[args.flash_freq]

        image.save(args.output + "0x00000.bin")
        data = e.load_section(".irom0.text")
        off = e.get_symbol_addr("_irom0_text_start") - 0x40200000
        assert off >= 0
        f = open(args.output + "0x%05x.bin" % off, "wb")
        f.write(data)
        f.close()

    elif args.operation == 'read_mac':
        mac = esp.read_mac()
        print 'MAC: %s' % ':'.join(map(lambda x: '%02x'%x, mac))

    elif args.operation == 'flash_id':
        flash_id = esp.flash_id()
        print 'Manufacturer: %02x' % (flash_id & 0xff)
        print 'Device: %02x%02x' % ((flash_id >> 8) & 0xff, (flash_id >> 16) & 0xff)

    elif args.operation == 'read_flash':
        print 'Please wait...'
        file(args.filename, 'wb').write(esp.flash_read(args.address, 1024, div_roundup(args.size, 1024))[:args.size])

    elif args.operation == 'erase_flash':
        esp.flash_erase()

    elif args.operation == 'wrap_stub':
        _, jstub = wrap_stub(args)
        print json.dumps(jstub)

    elif args.operation == 'run_stub':
        esp.run_stub(json.load(open(args.input)), args.params, read_output=True)
