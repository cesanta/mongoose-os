#!/usr/bin/env python

'''
mkid creates a configuration segment for Mongoose IoT ESP8266,
meant to be flashed at addr 0x10000. This tool is intended to be used
when you want to have control on the actual ID and/or you cannot use
MFT for some reason (e.g. paleolithic debian on ARM).
'''

import sys
import simplejson
import argparse
import hashlib

parser = argparse.ArgumentParser(description='Generate a device config')
parser.add_argument('--base', default="//api.cesanta.com/d", help="id base")
parser.add_argument('--id', help="device id")
parser.add_argument('--psk', help="key")

args = parser.parse_args()

if args.id is None or args.psk is None:
    print >>sys.stderr, "--id and --psk are mandatory"
    exit()

conf = dict(id="%s/%s" % (args.base, args.id), key=args.psk)
s = simplejson.dumps(conf)
print >>sys.stderr, "Conf:", s

h = hashlib.sha1()
h.update(s)
print h.digest() + s + '\0'
