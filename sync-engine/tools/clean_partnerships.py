#!/usr/bin/env python

import sys
import dbus

sys.path.insert(0, "..")
from SyncEngine.constants import *

try:
	engine = dbus.Interface(dbus.SessionBus().get_object(DBUS_SYNCENGINE_BUSNAME, DBUS_SYNCENGINE_OBJPATH), DBUS_SYNCENGINE_BUSNAME)
except:
	print "\nerror: unable to connect to running sync-engine"
	print "\nPlease ensure sync-engine is running before executing this command\n"
	sys.exit(1)

try:
	partnerships = engine.GetPartnerships()

	print "CAUTION: This command will delete ALL partnerships on the device\n"
	print "Please hit Y<enter> to continue, or <enter> to exit\n"

	s=raw_input("-")

	if s=='Y' or s=="y":

		print "Deleting all partnerships...\n"
		sys.stdout.flush()

		for pship in partnerships:
			id,guid,name,hostname,devicename,storetype,items = pship
			if storetype != PSHMGR_STORETYPE_AS:
				if storetype==PSHMGR_STORETYPE_EXCH:
					v="Server"
				else:
					v="Unknown"
				print "\nDelete %s partnership (%s)? (Y<enter> or just <enter> not to delete?)\n" % (name,v)
				s=raw_input("-")
				if s!='Y' and s!='y':
					continue
			engine.DeletePartnership(id,guid)

		print "Deletion complete\n"

	else:
		print "No partnerships have been deleted."
	
except dbus.DBusException,e:

	if e._dbus_error_name=="org.freedesktop.DBus.Python.SyncEngine.errors.Disconnected":
		print "error: No device connected"
	elif e._dbus_error_name=="org.freedesktop.DBus.Python.Exception":
		print e
	else:
		print "error: %s" % e._dbus_error_name
	sys.exit(1)
