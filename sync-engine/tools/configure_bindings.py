#!/usr/bin/env python

import sys
import dbus
import os
import tempfile

sys.path.insert(0, "..")
from SyncEngine.constants import *

try:
	engine = dbus.Interface(dbus.SessionBus().get_object(DBUS_SYNCENGINE_BUSNAME, DBUS_SYNCENGINE_OBJPATH), DBUS_SYNCENGINE_BUSNAME)
except:
	print "error: unable to connect to running sync-engine"
	exit(1)

try:
	bindings = engine.GetPartnershipBindings()
	itemtypes = engine.GetItemTypes()
except Exception,e:
	print "error: failed to query information from sync-engine"
	print "   %s" % e
	exit(1)

print
print "AVAILABLE HOST BINDINGS"
print "Index     Name         Device         SyncItems"
print "-----     ----         ------         ---------"

idx=0
for binding in bindings:
	id,guid,name,devicename,hostname,items = binding
	
	s="%d\t%s\t%s\t" % (idx,name,devicename)
	for item in items:
		s += "%s," % str(itemtypes[item])
		
	print s
	idx+=1

i=-1

print

while i==-1:
	s=raw_input("Select index of binding to configure, or empty to exit -")
	if s=="":
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

id,guid,name,hostname,devicename,items=bindings[i]
try:
	string=engine.QueryBindingConfig(id,guid)
except Exception,e:
	print "error: unable to query binding configuration"
	print "    %s" % e
	exit(1)

# Now create the file for config. This is nasty and there must be a better way
# than this....

try:
	f=tempfile.NamedTemporaryFile()
	n=f.name
	f.write(string)
	f.flush()
	os.system("vi %s" % n)
	f.seek(0)
	string=f.read()
	f.close()
except Exception, e:
	print "error: problem reading/writing temporary files"
	exit(1)

try:
	engine.SetBindingConfig(id,guid,string)
except Exception, e:
	print "error: unable to set binding configuration"
	exit(1)


