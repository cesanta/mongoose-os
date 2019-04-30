#!/usr/bin/env python3
#
# A utility to retrieve Device-to-Cloud messages
#
# Usage:
#
#   $ az_d2c_recv.py EVENT_HUB_NAME CONN_STR [offset]
#
# Both EH name and conn string can be obtained from the "Endpoints" section
# of the IoT Hub page. Note that Event Hub name is not the same as IoT Hub
# name and connectio string is not the same as used by the device.
#
# EH name looks like this: iothub-ehub-IOTHUBNAME-112232-5678abcdef
# Conn string starts with Endpoint=sb://ihsuproddbres019dednamespace... etc.
#
# For more info see here:
#   https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-devguide-messages-read-builtin
#
# This code is adapted from this script:
#   https://github.com/Azure/azure-event-hubs-python/blob/master/examples/recv.py

import os
import sys
import urllib

from azure.eventhub import EventHubClient, Receiver, Offset  # pip3 install azure-eventhub

hn = sys.argv[1]
ep = sys.argv[2]
OFFSET = Offset(sys.argv[3] if len(sys.argv) == 4 else "-1")

PARTITION = "0"
CONSUMER_GROUP = "$default"

def GetAMQPAddress(hn, ep):
    ep_parts = dict(kv.split("=", 1) for kv in ep.split(";"))
    return "amqps://%s:%s@%s/%s" % (
        urllib.parse.quote(ep_parts["SharedAccessKeyName"], safe=''),
        urllib.parse.quote(ep_parts["SharedAccessKey"], safe=''),
        ep_parts["Endpoint"].replace("sb://", "").replace("/", ""),
        hn
    )

client = EventHubClient(GetAMQPAddress(sys.argv[1], sys.argv[2]), debug=False)
try:
    receiver = client.add_receiver(CONSUMER_GROUP, PARTITION, prefetch=5000, offset=OFFSET)
    client.run()
    while True:
        for ev in receiver.receive():
            last_sn = ev.sequence_number
            did = str(ev.device_id, 'utf-8')
            print(ev.offset, ev.sequence_number, did, ev.message)

    client.stop()

except KeyboardInterrupt:
    pass
finally:
    client.stop()
