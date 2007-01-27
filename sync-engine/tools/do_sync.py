#!/usr/bin/env python

import gobject
import dbus
import dbus.glib
import sys

sys.path.insert(0, "..")
from engine.constants import *

engine = dbus.Interface(dbus.SessionBus().get_object(DBUS_SYNCENGINE_BUSNAME, DBUS_SYNCENGINE_OBJPATH), DBUS_SYNCENGINE_BUSNAME)

mainloop = gobject.MainLoop()

def synchronized_cb():
    print "Synchronization complete"
    gobject.idle_add(mainloop.quit)

engine.connect_to_signal("Synchronized", synchronized_cb)
engine.Synchronize()

mainloop.run()
