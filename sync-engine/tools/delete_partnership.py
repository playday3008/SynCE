#!/usr/bin/env python

import sys
import dbus

sys.path.insert(0, "..")
from engine.constants import *

engine = dbus.Interface(dbus.SessionBus().get_object(DBUS_SYNCENGINE_BUSNAME, DBUS_SYNCENGINE_OBJPATH), DBUS_SYNCENGINE_BUSNAME)

if len(sys.argv) != 2:
    print "Invalid argument count"
    sys.exit(1)

print "Deleting partnership..."
sys.stdout.flush()

engine.DeletePartnership(eval(sys.argv[1]))

print "done"
