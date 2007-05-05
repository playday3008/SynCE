#!/usr/bin/env python

import sys
import dbus

sys.path.insert(0, "..")
from SyncEngine.constants import *

engine = dbus.Interface(dbus.SessionBus().get_object(DBUS_SYNCENGINE_BUSNAME, DBUS_SYNCENGINE_OBJPATH), DBUS_SYNCENGINE_BUSNAME)

print "Deleting all partnerships..."
sys.stdout.flush()

for pship in engine.GetPartnerships():
    engine.DeletePartnership(pship[0])

print "done"
