#!/usr/bin/env python

import sys
import dbus

sys.path.insert(0, "..")
from SyncEngine.constants import *

try:
	engine = dbus.Interface(dbus.SessionBus().get_object(DBUS_SYNCENGINE_BUSNAME, DBUS_SYNCENGINE_OBJPATH), DBUS_SYNCENGINE_BUSNAME)
except:
	print "error: unable to attach to running sync-engine"
	exit(0)

try:
	pships = engine.GetPartnerships()
	item_types = engine.GetItemTypes()
except Exception, e:
	print "Failed to retrieve partnerships from sync-engine (%s)" % e


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
		exit(0)
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
try:
	id,guid,name,hostname,devicename,items = pships[i]
	engine.DeletePartnership(id,guid)
	print "Partnership deleted"
except Exception,e:	
	print "error: Failed to delete partnership"
	print "   %s" % e

