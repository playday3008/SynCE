#!/usr/bin/env python

import sys
import dbus

sys.path.insert(0, "..")
from SyncEngine.constants import *

engine = dbus.Interface(dbus.SessionBus().get_object(DBUS_SYNCENGINE_BUSNAME, DBUS_SYNCENGINE_OBJPATH), DBUS_SYNCENGINE_BUSNAME)

if len(sys.argv) != 3:
    print "Invalid argument count"
    sys.exit(1)

name_to_id = {}
for id, name in engine.GetItemTypes().items():
    name_to_id[name.lower()] = id

items = []
for token in sys.argv[2].split(","):
    name = token.lower()
    items.append(name_to_id[name])

print "Creating partnership..."
sys.stdout.flush()

engine.CreatePartnership(sys.argv[1], items)

print "done"
