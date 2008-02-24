#!/usr/bin/env python

import sys
import dbus
sys.path.insert(0, "..")
from SyncEngine.constants import *


def PrintUsage():
	print "Usage:"
	print "------\n"
	print "create_partnership \"<name>\" \"<sync_items>\""
	print "\nwhere:\n"
	print "<name> - the name of the partnership"
	print "<sync_items> - comma-separated list of items you wish to sync"
	print "               from one or more of Contacts,Calendar,Tasks,Files\n"
	print "In all cases both <name> and <sync_items> should be enclosed in"
	print "double quotation marks\n"
	print "For example:"
	print "  create_partnership \"Desktop\" \"Contacts,Calendar\" \n"


try:
	engine = dbus.Interface(dbus.SessionBus().get_object(DBUS_SYNCENGINE_BUSNAME, DBUS_SYNCENGINE_OBJPATH), DBUS_SYNCENGINE_BUSNAME)
	itypes = engine.GetItemTypes().items()
except:
	print "\nerror: unable to connect to running sync-engine"
	print "\nPlease ensure sync-engine is running before executing this command\n"
	exit(1)
	
if len(sys.argv) != 3:
	print "\nerror: Invalid argument count\n"
	PrintUsage()	
	exit(1)

name_to_id = {}
for id, name in itypes:
	name_to_id[name.lower()] = id

items = []
for token in sys.argv[2].split(","):
	name = token.lower()
	if name_to_id.has_key(name):
		items.append(name_to_id[name])
	else:
		print "\nerror: Invalid sync item type \"%s\"\n" % name
		PrintUsage()
		exit(1)

try:

	print "Creating partnership..."
	sys.stdout.flush()
	engine.CreatePartnership(sys.argv[1], items)
	print "Partnership created"
	
except dbus.DBusException,e:
	
	print "\nerror: failed to create partnership"
	if e._dbus_error_name=="org.freedesktop.DBus.Python.SyncEngine.errors.Disconnected":
		print "error: No device connected"
	elif e._dbus_error_name=="org.freedesktop.DBus.Python.SyncEngine.errors.InvalidArgument":
		print "error: invalid argument"
		PrintUsage()
	elif e._dbus_error_name=="org.freedesktop.DBus.Python.SyncEngine.errors.NoFreeSlots":
		print "error: Device has no free slots available in which to create the partnership"
	elif e._dbus_error_name=="org.freedesktop.DBus.Python.Exception":
		print e
	else:
		print "error: %s" % e._dbus_error_name
	exit(1)

