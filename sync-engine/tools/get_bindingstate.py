#!/usr/bin/env python

import dbus
import sys

sys.path.insert(0, "..")
from SyncEngine.constants import *

stab = { BSTATE_DEVNOTCONNECTED :  "device not connected",
         BSTATE_DEVNOTBOUND     :  "device connected but no valid partnership binding available",
	 BSTATE_DEVBOUND        :  "device connected and bound, ready to sync"
       }

try:
	engine = dbus.Interface(dbus.SessionBus().get_object(DBUS_SYNCENGINE_BUSNAME, DBUS_SYNCENGINE_OBJPATH), DBUS_SYNCENGINE_BUSNAME)
	item_types = engine.GetItemTypes()
except:
	print "\nerror: unable to connect to running sync-engine"
	print "\nPlease ensure sync-engine is running before executing this command\n"
	sys.exit(1)

try:
	pstate = engine.GetDeviceBindingState()

	if pstate in stab.keys():
		print "\nDevice state - %s\n" % stab[pstate]
	else:
		print "\nUnknown device state returned from sync-engine\n"

		
except dbus.DBusException,e:
	
	print "error: %s" % e._dbus_error_name
	sys.exit(0)



