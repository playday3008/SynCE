#!/usr/bin/env python

import dbus
import sys

sys.path.insert(0, "..")
from SyncEngine.constants import *

engine = dbus.Interface(dbus.SessionBus().get_object(DBUS_SYNCENGINE_BUSNAME, DBUS_SYNCENGINE_OBJPATH), DBUS_SYNCENGINE_BUSNAME)

item_types = engine.GetItemTypes()
i = 0
for id, name, host, items in engine.GetPartnerships():
    print "ID: %#x" % id
    print "Name: '%s'" % name
    print "Host: '%s'" % host
    str = "["
    for id in items:
        str += " %s" % item_types[id]
    str += " ]"
    print "Items: %s" % str
    i += 1

print "%d partnership(s)" % i
