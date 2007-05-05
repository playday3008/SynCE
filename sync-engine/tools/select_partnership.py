#!/usr/bin/env python

############################################################################
#    Copyright (C) 2007 Dr J A Gow 18/2/2007                               #
#                                                                          #
#    This program is free software; you can redistribute it and#or modify  #
#    it under the terms of the GNU General Public License as published by  #
#    the Free Software Foundation; either version 2 of the License, or     #
#    (at your option) any later version.                                   #
#                                                                          #
#    This program is distributed in the hope that it will be useful,       #
#    but WITHOUT ANY WARRANTY; without even the implied warranty of        #
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         #
#    GNU General Public License for more details.                          #
#                                                                          #
#    You should have received a copy of the GNU General Public License     #
#    along with this program; if not, write to the                         #
#    Free Software Foundation, Inc.,                                       #
#    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             #
############################################################################


import dbus
import sys

sys.path.insert(0, "..")
from SyncEngine.constants import *

engine = dbus.Interface(dbus.SessionBus().get_object(DBUS_SYNCENGINE_BUSNAME, DBUS_SYNCENGINE_OBJPATH), DBUS_SYNCENGINE_BUSNAME)

item_types = engine.GetItemTypes()
i = 0

ids = {}

for id, name, host, items in engine.GetPartnerships():
    ids[i] = id
    print "\n"
    print "Partnership %d:" % i
    print "\tID: %#x" % id
    print "\tName: '%s'" % name
    print "\tHost: '%s'" % host
    str = "["
    for id in items:
        str += " %s" % item_types[id]
    str += " ]"
    print "\tItems: %s" % str
    i += 1

print "\nTotal number of partnership(s) - %d\n" % i

if i > 0:

   v=None
   while v == None:

  	try:
    		v = int(raw_input("Partnership to make current?"))
	except:
    		print "bad value"
	
	if v > i or v < 0:
		v=None


   print "Setting current partnership to %d" % v

   res = engine.SetCurrentPartnership(ids[v])
    
   print "result -"
   if res == 0:
       print " done"
   else: 
       print " failed"

else:

    print "No partnerships to select."