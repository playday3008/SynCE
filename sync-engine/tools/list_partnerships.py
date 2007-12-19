#!/usr/bin/env python

import dbus
import sys

sys.path.insert(0, "..")
from SyncEngine.constants import *

engine = dbus.Interface(dbus.SessionBus().get_object(DBUS_SYNCENGINE_BUSNAME, DBUS_SYNCENGINE_OBJPATH), DBUS_SYNCENGINE_BUSNAME)

item_types = engine.GetItemTypes()
i = 0

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



