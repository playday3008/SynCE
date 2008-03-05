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
	pships = engine.GetPartnerships()
	item_types = engine.GetItemTypes()
	
	print
	print "            AVAILABLE DEVICE PARTNERSHIPS"
	print "Index     Name         Device      Host         SyncItems"
	print "-----     ----         ------      ----         ---------"
	print

	idx = 0

	for pship in engine.GetPartnerships():
	
	
		id,guid,name,hostname,devicename,items = pship
	
		s="%d\t%s\t%s\t%s\t" % (idx,name,devicename,hostname)
		s+="["
		for item in items:
			s += "%s " % str(item_types[item])
		s+="]"
		print s
		idx+=1

	i=-1
	
	while i==-1:
		s=raw_input("Select index of partnership to delete, or empty to exit -")
		if s=="":
			print "No partnership selected, exitting"
			sys.exit(0)
		try:
			i=int(s)
			if i > (idx-1):
				i=-1
				print "Index out of range (0-%d)" % idx-1
				continue
		except:
			print "Invalid input"
			pass
	
	print "Selected ",i


	print "Deleting partnership..."
	sys.stdout.flush()
	id,guid,name,hostname,devicename,items = pships[i]
	engine.DeletePartnership(id,guid)
	print "Partnership deleted"

except dbus.DBusException,e:
	
		if e._dbus_error_name=="org.freedesktop.DBus.Python.SyncEngine.errors.Disconnected":
			print "error: No device connected"
		elif e._dbus_error_name=="org.freedesktop.DBus.Python.SyncEngine.errors.SyncRunning":
			print "error: sync run in progress - please try again later"
		elif e._dbus_error_name=="org.freedesktop.DBus.Python.Exception":
			print e
		else:
			print "error: %s" % e._dbus_error_name
		sys.exit(0)

	
