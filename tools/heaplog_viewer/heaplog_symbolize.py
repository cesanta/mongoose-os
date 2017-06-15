#!/usr/bin/env python
#
# This script is needed to patch heaplog from the device: it converts all FW
# addresses there into corresponding symbolic names.
#
# Invoke with `--help` option to get list of options with description.
#
# See the file ./README.md for usage example.
#


import subprocess
import argparse
import re

CALL_TRACE_SIZE = 32;  # Must match the value in C file.

#-- create argument parser

parser = argparse.ArgumentParser()

parser.add_argument('-b', '--binary',
    default = '../../fw/platforms/esp8266/build/fw.out',
    required = False,
    help = 'Binary .out file (for example, fw.out)'
    )

parser.add_argument('-s', '--out-suffix',
    required = False,
    help = 'If provided, the output will be written into a file with provided suffix',
    )

parser.add_argument('heaplog',
    help = 'Heaplog file from device'
    )

#-- parse given arguments
myargs = parser.parse_args()

# address-to-symbol dictionary
symb_dict_by_addr = {}

def addr_to_symb(addr):
  ret = addr
  if addr in symb_dict_by_addr:
    ret = symb_dict_by_addr[addr]['name']
  return ret

# pattern to match symbols in `objdump` output
symbol_pattern = re.compile(
    '(?P<address>[0-9a-fA-F]{4,16})' +
    '\s+' +
    '(?P<local_global>[lg])' +
    '\s+' +
    '(?P<kind>[a-zA-Z]+)' +
    '\s+' +
    '(?P<section>[.a-zA-Z0-9_-]+)' +
    '\s+' +
    '(?P<size>[0-9a-fA-F]+)' +
    '\s+' +
    '(?P<name>[0-9a-zA-Z_]+)'
    )

calls_pattern = re.compile(
    '(.*\}\s+)' +
    '(?P<len>\d+)' +
    '\s+' +
    '(?P<start>\d+)' +
    '(\s+)?' +
    '(?P<addresses>[0-9a-fA-F ]*)'
    )

# -------------------------------

#-- run objdump as a child process, and pipe its stdout
out, err = subprocess.Popen(
    ['objdump', '-x', myargs.binary],
    stdout=subprocess.PIPE
    ).communicate()

# get list of all matches, and fill `symb_dict_by_addr` with them
matches = [m.groupdict() for m in symbol_pattern.finditer(out)]
for item in matches:
  symb_dict_by_addr[item['address']] = item

# open heaplog file, and replace all addresses with symbols there
heaplog_out = ''

with open(myargs.heaplog) as heaplog_file:
  prev_trace = []
  for num in range(CALL_TRACE_SIZE):
    prev_trace.append("00000000")

  for line in heaplog_file.readlines():
    m = re.match(calls_pattern, line)
    if m != None:
      prev_addr = "00000000"
      mdict = m.groupdict()
      start = int(mdict['start'])
      addresses = mdict['addresses'].split(" ")

      trace = prev_trace[:start]
      if start > 0:
        prev_addr = trace[start - 1]

      for addr in addresses:
        addr = prev_addr[:(8 - len(addr))] + addr
        trace.append(addr)
        prev_addr = addr

      for i in range(len(trace)):
        prev_trace[i] = trace[i]

      line = re.sub(calls_pattern, '\\1 ' + ' '.join([addr_to_symb(addr) for addr in trace]), line)
    heaplog_out += line

# output the result
if myargs.out_suffix == None:
  # just print to stdout
  print(heaplog_out)
else:
  # output to file
  filename = myargs.heaplog + myargs.out_suffix
  with open(filename, 'w') as heaplog_out_file:
    heaplog_out_file.write(heaplog_out)
  print('Output is written to the file "' + filename + '"')

