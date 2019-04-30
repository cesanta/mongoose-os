#!/usr/bin/python

import sys
import time

import pygatt  # pip install pygatt

MOS_DBG_SVC_UUID = "5f6d4f53-5f44-4247-5f53-56435f49445f"
DBG_LOG_CHAR_UUID = "306d4f53-5f44-4247-5f6c-6f675f5f5f30"

adapter = pygatt.GATTToolBackend()

try:
    adapter.start()
    addr = sys.argv[1]
    print "== Connecting to %s..." % addr
    device = adapter.connect(addr, timeout=5)
    print "== Connected, subscribing..."
    def print_value(unused_handle, value):
        print value
    device.subscribe(DBG_LOG_CHAR_UUID, print_value)
    print "== Subscribed"
    while True:
        time.sleep(1)
        if not adapter._connected_device:
            print "== Disconnected"
            break
finally:
    adapter.stop()
